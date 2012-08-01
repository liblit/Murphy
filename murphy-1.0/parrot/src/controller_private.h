

#ifndef _MALOS_CONTROLLER_PRIVATE_H_
#define _MALOS_CONTROLLER_PRIVATE_H_

#include "controller.h"
#include "MyString.h"
#include "extArray.h"
#include "HashTable.h"
#include "classad/classad_distribution.h"

// key needs to be name.pid.childnum 

struct gremlin_replay_entry {
	gremlin_replay_entry();
	int64_t current_entry;
	ExtArray<int64_t> invoked_index;
	ExtArray<int64_t> syscall_index;
};

struct gremlin_config_entry {
	gremlin_config_entry();

	MyString name;
	unsigned int seed;
	int percent_on;
	MyString constraint;
	ExprTree *constraint_tree;
};

struct gremlin_entry {
	gremlin_entry();
	~gremlin_entry();

	gremlin_config_entry *config;
	bool seed_initialized;
	unsigned int seed;

	MyString name;
	pid_t pid;
	unsigned int child_num;
	int64_t thissys_counter;	// how many times asked?
	int64_t evil_counter;		// how many times answered do evil?

	bool replay;
	unsigned int evil_array_size;
	//ExtArray<unsigned int> evil_thissys_index;
	//ExtArray<unsigned int> evil_allsys_index;
};

class controller {
	public:
	controller();
	void display();

	bool init(const char *config_file);
	bool process_config_line(MyString & line, struct gremlin_config_entry *);
	bool doevil(const char *name, const controller_context_t *context,
		const char *classad_context);
	bool setconfig(const char* config_line);

	private:
	HashTable<MyString, gremlin_entry*> gremlin_table;
	HashTable<MyString, gremlin_config_entry*> gremlin_config_table;
	HashTable<MyString, gremlin_replay_entry*> gremlin_replay_table;

	int gremlin_log_fd;

	bool read_log();
	void write_log_entry(const char *gname, 
		const controller_context_t *context, 
		const gremlin_entry *entry);
	

};

#endif
