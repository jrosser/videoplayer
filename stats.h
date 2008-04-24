#ifndef STATS_H_
#define STATS_H_

#include <map>
#include <string>

class Stats {
public:
		
	//each section of the player has some stats
	typedef std::map<std::string, std::string> inner_t;
	
	//there are potentially many sections
	typedef std::map<std::string, inner_t*> section_t;

	void addStat(const std::string &section_name, const std::string &stat_name, const std::string &stat_value);
	section_t::const_iterator get_section_begin();
	section_t::const_iterator get_section_end();

	//singleton access and construction
	static Stats &getInstance() {
		if(_instance == 0)
			_instance = new Stats();
		
		return *_instance;
	}
	
protected:
	Stats();

private:

	//singleton pointer
	static Stats* _instance;
	
	section_t sections;
};

#endif /*STATS_H_*/
