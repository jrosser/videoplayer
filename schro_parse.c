#include <string.h>

#include <stdio.h>

#include <schroedinger/schro.h>
#include <list>

enum SyncState {
	NOT_SYNCED=0,
	GOT_1PCP,
	TRY_SYNC,
	SYNCED,
	SYNCED_INCOMPLETEDU
};

void operator++(SyncState& s) { s = (SyncState)(s+1); }

struct SchroSyncState {
	enum SyncState state;
	int last_npo;
	int offset;
	std::list <SchroBuffer*> bufs;
};

static const char* syncstate_to_str[] = {
	"NOT_SYNCED", "GOT_1PCP", "TRY_SYNC", "SYNCED", "SYNCED_INCOMPLETEDU"
};

typedef std::list<SchroBuffer*> BS;

/* search @bufs@, starting at @offset@ to (sum (map length bufs))-10
 * for a parse_code_prefix.
 *
 * The -10 is to avoid situations where a match would be found, but
 * there isn't a complete parse_code_prefix in the bufferlist.
 */
bool schro_buflist_find_parseinfo(BS& bufs, int& offset)
{
	BS::iterator it = bufs.begin();
	int idx = offset;
	int idx_offset = 0;

	BS::iterator last = bufs.end();
	--last;

	/* find starting buffer and offset into it */
	while (idx >= (*it)->length) {
		idx -= (*it)->length;
		idx_offset += (*it)->length;
		++it;
	}

	do {
		int endskip = (it == last) ? 10 : 0;
		unsigned char* pos = (unsigned char*) memmem((*it)->data + idx, (*it)->length - idx - endskip, "BBCD", 4);
		if (pos) {
			/* found in this buffer */
			offset = idx_offset + pos - (*it)->data;
			return true;
		}

		if (endskip)
			break;

		/* deal with finding prefix that spans two buffers */
		/* part (1) look in this buffer */
		int i = 0;
		for (i = 0; i < 3; i++) {
			bool found = true;
			for (int j = 0; j < 3-i; j++) {
				if ("BBCD"[j] != *((*it)->data + (*it)->length - 3 + i)) {
					found = false;
				}
			}
			if (found)
				break;
		}

		idx = 0;
		idx_offset += (*it)->length;
		++it;

		/* part (2) look in the next buffer */
		for (int j = 0; j < 1+i; j++) {
			if ("BBCD"[3-i+j] != *((*it)->data + j)) {
				offset = idx_offset + j;
				return true;
			}
		}
	} while (it != bufs.end());
	return false;
}

int
schro_buflist_length(BS& bufs)
{
	int len = 0;
	for (BS::iterator it = bufs.begin(); it != bufs.end(); ++it)
		len += (*it)->length;
	return len;
}

SchroBuffer*
schro_buflist_new_subbuffer(BS& bufs, int offset, int len)
{
	BS::iterator it = bufs.begin();
	int idx = offset;

	/* find starting buffer and offset into it */
	while (idx >= (*it)->length) {
		idx -= (*it)->length;
		++it;
	}

	if (idx + len <= (*it)->length) {
		/* the found du is wholy in one buffer, form a subbuffer */
		return schro_buffer_new_subbuffer(*it, idx, len);
	}

	/* otherwise, aggregate to form a new buffer */
	SchroBuffer *b = schro_buffer_new_and_alloc(len);
	offset = 0;
	for (; it != bufs.end(); ++it, idx = 0) {
		int blen = (*it)->length - idx;
		int sublen = len < blen ? len : blen;
		memcpy(b->data + offset, (*it)->data + idx, sublen);
		offset += sublen;
		len -= sublen;
		if (!len)
			return b;
	}

	schro_buffer_unref(b);
	return NULL;
}

struct ParseInfo {
	int32_t next_parse_offset;
	int32_t prev_parse_offset;
	int parse_code;
};

bool schro_buflist_unpack_parseinfo(ParseInfo* pi, BS& bufs, int offset)
{
	if (offset < 0)
		return false;

	BS::iterator it = bufs.begin();
	int idx = offset;

	/* find starting buffer and offset into it */
	while (idx >= (*it)->length) {
		idx -= (*it)->length;
		++it;
	}

	const unsigned char* pu = (*it)->data + idx;

	if ((*it)->length - idx > 13) {
		/* fast path */
		if (pu[0] != 'B' || pu[1] != 'B' || pu[2] != 'C' || pu[3] != 'D')
			return false;

		pi->parse_code = pu[4];
		pi->next_parse_offset = pu[5] << 24 | pu[6] << 16 | pu[7] << 8 | pu[8];
		pi->prev_parse_offset = pu[9] << 24 | pu[10] << 16 | pu[11] << 8 | pu[12];
		return true;
	}

	/* slow path */
	memset(pi, 0, sizeof(ParseInfo));
	for (int i = 0; i < 13; i++) {
		if (it == bufs.end())
			return false;

		switch (i) {
		case 0: case 1: case 2: case 3:
			if (pu[i] != "BBCD"[i])
				return false;
			break;
		case 4:
			pi->parse_code = pu[4];
			break;
		case 5: case 6: case 7: case 8:
			pi->next_parse_offset |= pu[i] << ((8-i)*8);
			break;
		case 9: case 10: case 11: case 12:
			pi->prev_parse_offset |= pu[i] << ((12-i)*8);
			break;
		}

		if (idx + i + 1 == (*it)->length) {
			++it;
			idx = 0;
			pu = (*it)->data - i -1;
		}
	}
	/* slowpath complete */
	return true;
}

SchroBuffer*
schro_parse_getnextdu(SchroSyncState* s, SchroBuffer* b)
{
	if (b)
		s->bufs.push_back(b);

	if (schro_buflist_length(s->bufs) == 0)
		return NULL;

	ParseInfo pu;

	do {
		switch (s->state) {
		case NOT_SYNCED: /* -> GOT_1PCP */
		case GOT_1PCP:   /* -> TRY_SYNC */
			/* the only reason we care about 1PCP, is to stop reading
			 * a random stream filling up buf_list forever. (assuming
			 * that no 'BBCD' emulation occurs */
			/* XXX, not handled yet */
			if (schro_buflist_length(s->bufs) - s->offset <= 1)
				/* there must be at least one byte avaliable in bufs */
				return NULL;
			s->offset++;
			if (schro_buflist_find_parseinfo(s->bufs, s->offset))
				++s->state;
			break;
		case TRY_SYNC: { /* -> (SYNCED | GOT_1PCP) */
			ParseInfo pu1;
			bool a = schro_buflist_unpack_parseinfo(&pu1, s->bufs, s->offset);
			bool b = schro_buflist_unpack_parseinfo(&pu, s->bufs, s->offset - pu1.prev_parse_offset);
			if (!a || !b || (pu1.prev_parse_offset != pu.next_parse_offset)) {
				s->state = GOT_1PCP;
				break;
			}
			s->last_npo = pu.next_parse_offset;
			s->state = SYNCED;
			break;
		}
		case SYNCED: {  /* -> (SYNCED | GOT_1PCP) */
			bool a = schro_buflist_unpack_parseinfo(&pu, s->bufs, s->offset);
			if (!a || (s->last_npo != pu.prev_parse_offset)) {
				s->state = GOT_1PCP;
				break;
			}
			s->offset += pu.next_parse_offset;
			s->last_npo = pu.next_parse_offset;
			s->state = SYNCED;
			break;
		}
		case SYNCED_INCOMPLETEDU:
			/* reread pi */
			schro_buflist_unpack_parseinfo(&pu, s->bufs, s->offset - s->last_npo);
			s->state = SYNCED; /* try again */
		default: break;
		}
	} while (SYNCED != s->state);

	if (SYNCED != s->state)
		return NULL;

	/* synced, extract a data unit */
	SchroBuffer* du;
	du = schro_buflist_new_subbuffer(s->bufs
	                                ,s->offset - pu.next_parse_offset
	                                ,pu.next_parse_offset);
	if (!du) {
		s->state = SYNCED_INCOMPLETEDU;
		return NULL;
	}

#if 0
		fprintf(stderr, "got: (from offset:%d len:%d\n"
		       " %02x %02x %02x %02x\n %02x\n"
		       " %02x %02x %02x %02x\n %02x %02x %02x %02x\n"
		       ,s->offset - pu.next_parse_offset, pu.next_parse_offset
		       ,du->data[0], du->data[1], du->data[2], du->data[3]
		       ,du->data[4]
		       ,du->data[5], du->data[6], du->data[7], du->data[8]
		       ,du->data[9], du->data[10], du->data[11], du->data[12]
		       );
#endif
	/* clean up bufs */
	/* see if offset is > any buffers in buf */
	for (BS::iterator it = s->bufs.begin(); it != s->bufs.end();) {
		if ((*it)->length > s->offset)
			break;
		s->offset -= (*it)->length;
		++it;
		b = s->bufs.front();
		schro_buffer_unref(b);
		s->bufs.pop_front();
	}

	return du;
}
