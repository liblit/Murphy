
#include <stdio.h>
#include <stdlib.h>
#include "controller.h"
#include "controller_private.h"
#include "pfs_dispatch.h"  // for suspend_and_gdb

// globals set in main via command-line
extern char *gremlin_log_file;
extern char *gremlin_replay_file;
extern bool gremlin_replay_gdb;
extern char *murphy_config_file;

static controller* _controller = NULL;

/*******************************/

/* #define STANDALONEDEBUG 1 */

#ifndef STANDALONEDEBUG
	extern "C" {
		#include "debug.h"
	}
	#define DLEVEL D_MURPHY,
#else

#define DLEVEL
#define debug printf


int
main()
{
	controller c;
	controller_context_t context;

	context.syscall_count = 1;
	context.pid = 66;
	context.childnum = 1;
	context.ioctl_meta = "In foo()";

	c.init(NULL);
	

	controller_doevil("read",&context,NULL);
	controller_doevil("read",&context,NULL);
	controller_doevil("write",&context,NULL);
	controller_doevil("write",&context,NULL);
	controller_doevil("foo",&context,NULL);
	controller_doevil("bar",&context,NULL);


	controller_setconfig("default:80");

	controller_doevil("read",&context,NULL);
	controller_doevil("read",&context,NULL);
	controller_doevil("write",&context,NULL);
	controller_doevil("write",&context,NULL);
	controller_doevil("foo",&context,NULL);
	controller_doevil("foo",&context,NULL);
	controller_doevil("bar",&context,NULL);
	controller_doevil("bar",&context,NULL);

	context.pid = 67;
	controller_doevil("read",&context,NULL);
	controller_doevil("read",&context,NULL);
	controller_doevil("write",&context,NULL);
	controller_doevil("write",&context,NULL);
	controller_doevil("foo",&context,NULL);
	controller_doevil("foo",&context,NULL);
	controller_doevil("bar",&context,NULL);
	controller_doevil("bar",&context,NULL);

	return 0;
}

#endif 
/********************************/

/** PUBLIC INTERFACE **/

bool
controller_doevil(const char *gremlin_name,
	const controller_context_t *context,
	const char *classad_context)
{
	if (! _controller ) {
		_controller = new controller;
		_controller->init(NULL);
	}

	return _controller->doevil(gremlin_name,context,classad_context);
}

bool
controller_setconfig(const char* config_line)
{
	if (! _controller ) {
		_controller = new controller;
		_controller->init(NULL);
	}

	return _controller->setconfig(config_line);
}

bool
controller_init()
{
	if (! _controller ) {
		_controller = new controller;
	}
	return _controller->init(NULL);
}

/** IMPLEMENTATION **/

int 
controller_config_print(gremlin_config_entry *entry)
{
	debug ( DLEVEL 
			"controller config entry: "
			"name=%s, seed=%u, percent_on=%d, constraint=%s\n",
			entry->name.Value(), entry->seed, entry->percent_on,
			entry->constraint.Value());
	return 1;
}

void
controller::display()
{
	gremlin_config_table.walk( controller_config_print );

}

gremlin_config_entry::gremlin_config_entry()
{
	static unsigned int num_entries = 0;
	seed = ++num_entries;
	percent_on = 0;
	constraint_tree = NULL;
}


controller::controller() :
	gremlin_table(200, hashFuncMyString,rejectDuplicateKeys),
	gremlin_config_table(20,hashFuncMyString,updateDuplicateKeys),
	gremlin_replay_table(200, hashFuncMyString,rejectDuplicateKeys)
{
	gremlin_log_fd = 0;
}

bool
controller::setconfig(const char* config_line)
{
	gremlin_config_entry *entry = NULL;
	bool newlyCreated = false;
	MyString line(config_line);
	const char *tmp;
	bool ret_value = false;

	line.chomp();
	line.trim();
	if ( line.IsEmpty() || line[0] == '#' ) {
		// comment line or blank line - we're done
		// return false to indicate config entry invalid
		return false;
	}
	line.Tokenize();
	// Read gremlin name -- required
	tmp = line.GetNextToken(":",false);
	if ( !tmp || !tmp[0] ) {
		return false;
	}
	MyString name( tmp );

	gremlin_config_table.lookup( name, entry );

	if (!entry) {
		entry = new gremlin_config_entry;
		newlyCreated = true;
	}

	// If line processes properly, then insert in the table
	// if we just allocated it, else delete it if we just 
	// allocated it.
	if ( process_config_line(line,entry) ) {
		if (newlyCreated)
			gremlin_config_table.insert(entry->name,entry);
		ret_value =  true;
	} else {
		if (newlyCreated)
			delete entry;
		ret_value =  false;
	}
	
	debug ( DLEVEL 
			"controller exiting setconfig(%s) ret=%s\n", 
			config_line,
			ret_value ? "true" : "false");
	display();

	return ret_value;
}

bool
controller::process_config_line(MyString & line, 
		struct gremlin_config_entry * entry)
{
	const char *tmp;
	int seed = 0;
	const char *constraint = NULL;
	ClassAdParser parser;
	ExprTree *constraint_tree = NULL;

	line.chomp();
	line.trim();

	if ( line.IsEmpty() || line[0] == '#' ) {
		// comment line or blank line - we're done
		// return false to indicate config entry invalid
		return false;
	}

	line.Tokenize();

	// Read gremlin name -- required
	tmp = line.GetNextToken(":",false);
	if ( !tmp || !tmp[0] ) {
		return false;
	}
	MyString name( tmp );
	
	// Read activation percentage -- required
	tmp = line.GetNextToken(":",false);
	if ( !tmp || !tmp[0] ) {
		return false;
	}
	int percent = atoi(tmp);

	// Read seed -- optional
	tmp = line.GetNextToken(":",false);
	if ( !tmp || !tmp[0] ) {
		seed = 0;
	} else {
		seed = atoi(tmp);
	}

	// Read constraint expr -- optional
	tmp = line.GetNextToken(":",false);
	if ( !tmp || !tmp[0] ) {
		constraint = NULL;
	} else {
		constraint = tmp;
		// Attmpt to parse the contrainst expr
		if ( !(constraint_tree = parser.ParseExpression(tmp)) ) {
			debug ( DLEVEL 
			  "controller gremlin %s ignored, failed to parse constraint %s\n", 
			  name.Value(),tmp);
			return false;
		}
	}

	// If we made it here, all parsed ok
	
	entry->name = name;
	// for seed, a value of 0 means use default seed which
	// is a unique number for each gremlin
	if ( seed > 0 )
		entry->seed = seed;
	if ( percent >= 0 && percent < 101 ) 
		entry->percent_on = percent;
	if ( constraint ) {
		entry->constraint = constraint;
		entry->constraint.trim();
		if (entry->constraint_tree) delete entry->constraint_tree;
		entry->constraint_tree = constraint_tree;
	}

	return true;
}

gremlin_entry::gremlin_entry()
{
	config = NULL;
	seed_initialized = false;
	seed = 0;
	pid = 0;
	child_num = 0;
	thissys_counter = 0;
	evil_counter = 0;
	replay = false;
	evil_array_size = 0;
}

gremlin_entry::~gremlin_entry()
{
}

gremlin_replay_entry::gremlin_replay_entry()
{
	current_entry = 0;
}

bool
controller::read_log()
{
	FILE *fp = NULL;
	MyString line;
	MyString key;
	int result;
	char gname[80];
	unsigned int childnum;
	int64_t invoked_i, syscall_i, evil_counter;
	gremlin_replay_entry *replay_entry = NULL;
	unsigned int linenum = 0;

	if (!gremlin_replay_file) return false;

	fp = fopen(gremlin_replay_file,"r");
	if (!fp) {
		fatal("Failed to open gremlin replay file %s: %s\n",
			gremlin_replay_file, strerror(errno));
		return false;
	}
	while (line.readLine(fp)) {
		line.chomp();
		line.trim();
		linenum++;
		if ( line.IsEmpty() || line[0] == '#' ) continue;
		result = sscanf(line.Value(), "%s %lu %u %lu %lu Meta:\n",
			gname, &evil_counter, &childnum,
			&invoked_i, &syscall_i);
		if ( result != 5 ) {
			fatal("Failed to parse replay log - error on line %u\n",
				linenum);
		}
		/*
	debug ( DLEVEL "controller replay line: %s %lu %u %lu %lu Meta:\n",
			gname, evil_counter, childnum,
			invoked_i, syscall_i);
		*/

		key.sprintf("%s.%u",gname,childnum);
		replay_entry = NULL;
		gremlin_replay_table.lookup(key,replay_entry);
		if (replay_entry == NULL ) {
			replay_entry = new gremlin_replay_entry;
			gremlin_replay_table.insert(key,replay_entry);
			debug( DLEVEL "controller entered replay_entry with key %s\n",key.Value());
		}
		replay_entry->invoked_index.add(invoked_i);
		replay_entry->syscall_index.add(syscall_i);
	}
	fclose(fp);

	debug ( DLEVEL "controller read %u lines from replay log\n", linenum);

	return true;
}

void
controller::write_log_entry(const char *gname, 
	const controller_context_t *context, 
	const gremlin_entry *entry)
{
	char buf[8092];
	static bool first_time = true;
	int len, written, ret_val;

	if ( gremlin_log_fd < 1 ) return;

	if (first_time) {
		strcpy(buf,"#GremlinName EvilCount ChildNum InvokedCount SyscallCount\n");
		len = strlen(buf);
		written = 0;

		while (written < len) {
			ret_val = write(gremlin_log_fd,&buf[written],len-written);
			if ( ret_val < 0 && errno == EINTR ) continue;
			if ( ret_val > 0 ) written += ret_val;
		}
		first_time = false;
	}

	/* Gremlin EvilCount ChildNum InvokedCount SyscallCount Meta: */
	snprintf(buf, sizeof(buf), "%s %lu %u %lu %lu Meta: %s\n",
		gname, entry->evil_counter, context->childnum,
		entry->thissys_counter, context->syscall_count,
		context->ioctl_meta ? context->ioctl_meta : "" );

	len = strlen(buf);
	written = 0;

	while (written < len) {
		ret_val = write(gremlin_log_fd,&buf[written],len-written);
		if ( ret_val < 0 && errno == EINTR ) continue;
		if ( ret_val > 0 ) written += ret_val;
	}
}

bool
controller::doevil(const char *gname, const controller_context_t *context,
		const char *classad_context)
{
	bool doevil = false;
	bool evalResult = true;
	MyString key;
	MyString name(gname);
	gremlin_entry *entry = NULL;
	gremlin_config_entry *config = NULL;
	int random = 0;

	// key needs to be name.pid.childnum
	key.sprintf("%s.%u.%u",gname,context->pid,context->childnum);

	gremlin_table.lookup(key,entry);
	
	if ( entry == NULL ) {
		entry = new gremlin_entry;
		gremlin_table.insert(key,entry);
		entry->name = name;
		entry->pid = context->pid;
		entry->child_num = context->childnum;
	}

	config = entry->config;
	if ( config == NULL ) {
		gremlin_config_table.lookup( name, config );
		// if we find a config for gremlin, keep a copy
		// in the entry. if not, look up the default config,
		// but do NOT keep a copy in the entry in case the
		// user ioctls a new config entry for this gremlin.
		if ( config ) {
			entry->config = config;
		} else {
			gremlin_config_table.lookup( MyString("default"), config );
		}
		// initialize seed from config table. we only do this the first
		// time this gremlin is referenced in this process.
		if ( !entry->seed_initialized ) {
			entry->seed = config->seed;
			entry->seed_initialized = true;
		}
	}

	// Finally, decide what to do!
	if ( gremlin_replay_file ) {
		static bool already_warned = false;
		MyString key;
		gremlin_replay_entry *replay_entry = NULL;

		key.sprintf("%s.%u",gname,context->childnum);
		gremlin_replay_table.lookup(key,replay_entry);
		if (replay_entry) { 
			debug( DLEVEL "controller found replay_entry with key %s\n",key.Value()); 
		}
		if ( replay_entry && 
		   	 (entry->thissys_counter == replay_entry->invoked_index[replay_entry->current_entry]) )
		{
			if (!already_warned &&
				(context->syscall_count !=
				replay_entry->syscall_index[replay_entry->current_entry]))
			{
				debug ( D_NOTICE,
					"controller WARNING non-deterministic replay: "
					"invocation %lu of %s\n",
					entry->thissys_counter, key.Value());
				already_warned = true;
			}
			doevil = true;
			replay_entry->current_entry += 1;
			if ( replay_entry->current_entry == 
				 replay_entry->invoked_index.length() ) 
			{
				// if we are done replaying this entry, remove from hash table
				gremlin_replay_table.remove(key);
				// if this was the last entry, fire up gdb if requested
				if ( gremlin_replay_gdb && gremlin_replay_table.getNumElements() == 0 ) {
					suspend_and_gdb();
					debug(D_MURPHY, "end of replay log -- will invoke gdb on exit\n");
				}
			}
		}
	} else
	if ( config->percent_on == 0 ) {
		doevil = false;
	} else 
	if ( config->percent_on == 100 ) {
		doevil = true;
	} else {
		random = rand_r(&entry->seed) % 100 ;
		doevil = random < config->percent_on ? true : false;
	}

	// If above decided to do evil, make sure constraint holds
	if ( doevil && config->constraint_tree ) {
		ClassAd ad;
		ClassAdParser parser;
		// Insert gremlin context into ad if we have one
		if (classad_context &&
			!parser.ParseClassAd(classad_context,ad,true ))
		{
			debug ( DLEVEL "controller doEvil %s classad context bad (%s)\n",
			key.Value(), classad_context );

		}
		// Now insert gremlin-indepent context attributes
		ad.InsertAttr("Gremlin",gname);
		ad.InsertAttr("SyscallCount",(int)context->syscall_count);
		ad.InsertAttr("InvokedCount",(int)entry->thissys_counter);
		ad.InsertAttr("EvilCount",(int)entry->evil_counter);
		ad.InsertAttr("Pid",(int)context->pid);
		ad.InsertAttr("ChildNum",(int)context->childnum);
		ad.InsertAttr("SyscallName",context->syscall_name);
		ad.InsertAttr("SyscallNum",(int)context->syscall_number);
		MyString meta(context->ioctl_meta);
		ad.InsertAttr("Meta",meta.Value());
		// Now evaluate the contraint
		ExprTree *private_copy = config->constraint_tree->Copy();
		ad.Insert("Constraint",private_copy);
		ad.EvaluateAttrBool("Constraint",evalResult);
		if (!evalResult) {
			doevil = false;
		}
		// Display some useful debug info into log
		ClassAdUnParser unp;
		std::string displayad;
		unp.Unparse(displayad,&ad);
		debug ( DLEVEL "controller evalResult=%s %s\n",
			evalResult ? "true" : "false", displayad.c_str() );

	}


	if ( config->percent_on ) {
		debug ( DLEVEL "controller doEvil %s %s (%d<%d) constraint=%s\n",
			key.Value(), doevil ? "true" : "false", 
			random, config->percent_on,
			evalResult ? "true" : "false");
	}

	// write out to our gremlin log, if needed
	if (doevil) {
		write_log_entry(gname, context, entry);
	}

	// Update some statistics for our report
	entry->thissys_counter++;
	if ( doevil ) {
		entry->evil_counter++;
	}

	return doevil;
}

bool
controller::init(const char *config_file)
{
	bool ret_value = true;
	FILE *fp;
	MyString line;
	bool line_good;
	gremlin_config_entry * entry;
	static bool already_initialized = false;

	// Check if we've already been here
	if ( already_initialized ) 
		return true;
	else
		already_initialized = true;

	// Insert a default entry; this can be overwritten by
	// a default given by the user below
	line = "default:0";
	entry = new gremlin_config_entry;
	if ( process_config_line(line,entry) ) {
		gremlin_config_table.insert(entry->name,entry);
	}

	if ( ! config_file ) {
		// use config specified on command line
		config_file = murphy_config_file;
	}

	if ( ! config_file ) {
		// if no command line option used, check the environment
		config_file = getenv("MURPHY_CONFIG");
	}

	if ( config_file ) {
		entry = new gremlin_config_entry;
		fp = fopen(config_file,"r");
		while (fp && line.readLine(fp)) {
			line_good = process_config_line(line,entry);
			if (ret_value && !line_good) ret_value = false;
			if ( line_good ) {
				// if line parsed, insert into hash table
				gremlin_config_table.insert(entry->name,entry);
				entry = new gremlin_config_entry;
			}
		}
		fclose(fp);
	}

	if ( gremlin_log_file ) {
		gremlin_log_fd = 
			open(gremlin_log_file,
				O_APPEND|O_CREAT|O_LARGEFILE|O_TRUNC|O_WRONLY,
				0644);
		if ( gremlin_log_file < 0 ) {
			fatal("failed to open gremlin log file %s : %s\n",
				gremlin_log_file, strerror(errno));
		}
	}

	if (gremlin_replay_file) {
		read_log();
	}

	debug ( DLEVEL 
			"controller exiting init() ret=%s config_file=%s\n", 
			ret_value ? "true" : "false",
			config_file ? config_file : "(null)" );
	display();

	return ret_value;
}



