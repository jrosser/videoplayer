#include "stats.h"

#include <sstream>

using namespace std;

//singleton pointer initialisation
Stats *Stats::_instance = 0;

Stats::Stats()
{

}

Stats::Section* Stats::newSection(const string &section_name, void* ptr)
{
	Section *s = new Stats::Section();

	std::stringstream ss;
	ss << section_name << "[" << ptr << "]";

	QMutexLocker lock(&mutex);
	sections[ss.str()] = s;
	return s;
}

void Stats::Section::addStat(const string& name, const string& value)
{
	QMutexLocker lock(&mutex);
	key_to_val[name] = value;
}
