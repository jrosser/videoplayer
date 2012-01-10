#ifndef STATS_H_
#define STATS_H_

#include <map>
#include <string>
#include <sstream>

#include <QMutex>

class Stats {
public:
	class Section {
	public:
		/* map keys to values */
		typedef std::map<std::string, std::string> key_to_val_t;
		typedef key_to_val_t::const_iterator const_iterator;
		const_iterator begin() { return key_to_val.begin(); }
		const_iterator end() { return key_to_val.end(); }

		/**
		 * mutex that should be held when attempting to touch @key_to_val.
		 */
		QMutex mutex;

		/* add a key and value to the section, replacing any previous value */
		void addStat(const std::string& name, const std::string& value);

	private:
		key_to_val_t key_to_val;
	};

	//there are potentially many sections
	typedef std::map<std::string, Section*> section_t;
	typedef section_t::const_iterator const_iterator;
	const_iterator begin() { return sections.begin(); }
	const_iterator end() { return sections.end(); }

	//singleton access and construction
	static Stats &getInstance() {
		if(_instance == 0)
			_instance = new Stats();
		
		return *_instance;
	}

	Section* newSection(const std::string &section_name, void* ptr = 0);

	/**
	 * mutex that should be held when attempting to touch @sections.
	 */
	QMutex mutex;

protected:
	Stats();

private:
	//singleton pointer
	static Stats* _instance;

	section_t sections;
};

template<typename T>
void addStat(Stats::Section& section, const std::string &name, T d)
{
	std::stringstream ss;
	ss << d;

	section.addStat(name, ss.str());
}

template<typename T>
void addStat(Stats::Section& section, const std::string &name, T d, const std::string &unit)
{
	std::stringstream ss;
	ss << d << " " << unit;

	section.addStat(name, ss.str());
}

#endif /*STATS_H_*/
