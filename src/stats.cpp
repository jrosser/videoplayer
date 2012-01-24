
#include <QThread>

#include "stats.h"

#include <sstream>

using namespace std;

//singleton pointer initialisation
Stats *Stats::_instance = 0;

Stats::Stats()
{

}

Stats::Section& Stats::getSection(const string &section_name, void* ptr)
{
	Stats& stats = getInstance();

	if (!ptr)
		ptr = QThread::currentThread();

	std::stringstream ss;
	ss << section_name << "[" << ptr << "]";

	QMutexLocker lock(&stats.mutex);
	if (stats.sections.find(ss.str()) == stats.sections.end())
		return *(stats.sections[ss.str()] = new Stats::Section());
	else
		return *stats.sections[ss.str()];
}

void Stats::Section::addStat(const string& name, const string& value)
{
	QMutexLocker lock(&mutex);
	key_to_val[name] = value;
}
