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
	std::stringstream ss;
	ss << section_name << "[" << ptr << "]";

	QMutexLocker lock(&mutex);
	if (sections.find(ss.str()) == sections.end())
		return sections[ss.str()] = new Stats::Section();
	else
		return sections[ss.str()];
}

void Stats::Section::addStat(const string& name, const string& value)
{
	QMutexLocker lock(&mutex);
	key_to_val[name] = value;
}
