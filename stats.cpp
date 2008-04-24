#include "stats.h"

#include <iostream>

using namespace std;

//singleton pointer initialisation
Stats *Stats::_instance = 0;

Stats::Stats()
{

}

void Stats::addStat(const string &section_name, const string &stat_name, const string &stat_value)
{
	inner_t *sec = sections[section_name];

	if(!sec) {
		//create the section if it does not exist
		sec = sections[section_name] = new inner_t();
	}

	//insert the stat into the section
	(*sec)[stat_name] = stat_value;
}

Stats::section_t::const_iterator Stats::get_section_begin()
{
	return sections.begin();
}

Stats::section_t::const_iterator Stats::get_section_end()
{
	return sections.end();
}

#ifdef HARNESS
int main(void)
{
	Stats &s = Stats::getInstance();
	
	s.addStat("S1", "first", "1");
	s.addStat("S1", "first", "5");
	s.addStat("S1", "second", "2");
	
	s.addStat("foo", "xfirst", "1");
	s.addStat("foo", "xsecond", "2");
	
	Stats::section_t::const_iterator it = s.get_section_begin();
	while(it != s.get_section_end())
	{ 
		std::cout << (*it).first << std::endl;  //name of the section
	
		Stats::inner_t::const_iterator it2 = (*it).second->begin();
		
		while(it2 != (*it).second->end()) {
			std::cout << (*it2).first << " = " << (*it2).second << std::endl;
			it2++;
		}

		it++;
	}
}
#endif
