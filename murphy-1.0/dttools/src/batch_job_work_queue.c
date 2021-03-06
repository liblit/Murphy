#include "batch_job.h"
#include "batch_job_internal.h"
#include "work_queue.h"
#include "debug.h"
#include "stringtools.h"
#include "macros.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


void specify_work_queue_task_files(struct work_queue_task *t, const char *input_files, const char *output_files)
{
	char *f, *p, *files;

	if(input_files) {
		files = strdup(input_files);
		f = strtok(files, " \t,");
		while(f) {
			p = strchr(f, '=');
			if(p) {
				*p = 0;
				work_queue_task_specify_input_file(t, f, p + 1);
				debug(D_DEBUG, "local file %s is %s on remote system:", f, p + 1);
				*p = '=';
			} else {
				work_queue_task_specify_input_file(t, f, f);
			}
			f = strtok(0, " \t,");
		}
		free(files);
	}

	if(output_files) {
		files = strdup(output_files);
		f = strtok(files, " \t,");
		while(f) {
			p = strchr(f, '=');
			if(p) {
				*p = 0;
				work_queue_task_specify_output_file(t, f, p + 1);
				debug(D_DEBUG, "remote file %s is %s on local system:", f, p + 1);
				*p = '=';
			} else {
				work_queue_task_specify_output_file(t, f, f);
			}
			f = strtok(0, " \t,");
		}
		free(files);
	}
}

void specify_work_queue_task_shared_files(struct work_queue_task *t, const char *input_files, const char *output_files)
{
	char *f, *p, *files;

	if(input_files) {
		files = strdup(input_files);
		f = strtok(files, " \t,");
		while(f) {
			char fname[WORK_QUEUE_LINE_MAX];
			p = strchr(f, '=');
			if(p) {
				*p = 0;
			}

			if(f[0] != '/') {
				char tmp[WORK_QUEUE_LINE_MAX];
				getcwd(tmp, WORK_QUEUE_LINE_MAX);
				strcat(tmp, "/");
				strcat(tmp, f);
				string_collapse_path(tmp, fname, 1);
			} else {
				strcpy(fname, f);
			}

			if(p) {	
				work_queue_task_specify_file(t, f, p + 1, WORK_QUEUE_INPUT, WORK_QUEUE_CACHE|WORK_QUEUE_THIRDGET);
				debug(D_DEBUG, "shared file %s is %s on remote system:", f, p + 1);
				*p = '=';
			} else {
				work_queue_task_specify_file(t, fname, fname, WORK_QUEUE_INPUT, WORK_QUEUE_CACHE|WORK_QUEUE_THIRDGET);
			}
			f = strtok(0, " \t,");
		}
		free(files);
	}

	if(output_files) {
		files = strdup(output_files);
		f = strtok(files, " \t,");
		while(f) {
			char fname[WORK_QUEUE_LINE_MAX];
			p = strchr(f, '=');
			if(p) {
				*p = 0;
			}

			if(f[0] != '/') {
				char tmp[WORK_QUEUE_LINE_MAX];
				getcwd(tmp, WORK_QUEUE_LINE_MAX);
				strcat(tmp, "/");
				strcat(tmp, f);
				string_collapse_path(tmp, fname, 1);
			} else {
				strcpy(fname, f);
			}

			if(p) {	
				work_queue_task_specify_file(t, fname, p + 1, WORK_QUEUE_OUTPUT, WORK_QUEUE_THIRDPUT);
				debug(D_DEBUG, "shared file %s is %s on remote system:", f, p + 1);
				*p = '=';
			} else {
				work_queue_task_specify_file(t, fname, fname, WORK_QUEUE_OUTPUT, WORK_QUEUE_THIRDPUT);
			}
			f = strtok(0, " \t,");
		}
		free(files);
	}
}

batch_job_id_t batch_job_submit_work_queue(struct batch_queue *q, const char *cmd, const char *args, const char *infile, const char *outfile, const char *errfile, const char *extra_input_files, const char *extra_output_files)
{
	struct work_queue_task *t;
	char *full_command;

	if(infile)
		full_command = (char *) malloc((strlen(cmd) + strlen(args) + strlen(infile) + 5) * sizeof(char));
	else
		full_command = (char *) malloc((strlen(cmd) + strlen(args) + 2) * sizeof(char));

	if(!full_command) {
		debug(D_DEBUG, "couldn't create new work_queue task: out of memory\n");
		return -1;
	}

	if(infile)
		sprintf(full_command, "%s %s < %s", cmd, args, infile);
	else
		sprintf(full_command, "%s %s", cmd, args);

	t = work_queue_task_create(full_command);

	free(full_command);

	if(q->type == BATCH_QUEUE_TYPE_WORK_QUEUE_SHAREDFS) {
		if(infile)
			work_queue_task_specify_file(t, infile, infile, WORK_QUEUE_INPUT, WORK_QUEUE_CACHE|WORK_QUEUE_THIRDGET);
		if(cmd)
			work_queue_task_specify_file(t, cmd, cmd, WORK_QUEUE_INPUT, WORK_QUEUE_CACHE|WORK_QUEUE_THIRDGET);

		specify_work_queue_task_shared_files(t, extra_input_files, extra_output_files);
	} else {
		if(infile)
			work_queue_task_specify_input_file(t, infile, infile);
		if(cmd)
			work_queue_task_specify_input_file(t, cmd, cmd);

		specify_work_queue_task_files(t, extra_input_files, extra_output_files);
	}

	work_queue_submit(q->work_queue, t);

	if(outfile) {
		itable_insert(q->output_table, t->taskid, strdup(outfile));
	}

	return t->taskid;
}

batch_job_id_t batch_job_submit_simple_work_queue(struct batch_queue *q, const char *cmd, const char *extra_input_files, const char *extra_output_files)
{
	struct work_queue_task *t;

	t = work_queue_task_create(cmd);

	if(q->type == BATCH_QUEUE_TYPE_WORK_QUEUE_SHAREDFS) {
		specify_work_queue_task_shared_files(t, extra_input_files, extra_output_files);
	} else {
		specify_work_queue_task_files(t, extra_input_files, extra_output_files);
	}

	work_queue_submit(q->work_queue, t);

	return t->taskid;
}

batch_job_id_t batch_job_wait_work_queue(struct batch_queue * q, struct batch_job_info * info, time_t stoptime)
{
	static FILE *logfile = 0;
	struct work_queue_stats s;

	int timeout, taskid = -1;

	if(!logfile) {
		logfile = fopen(q->logfile, "a");
		if(!logfile) {
			debug(D_NOTICE, "couldn't open logfile %s: %s\n", q->logfile, strerror(errno));
			return -1;
		}
	}

	if(stoptime == 0) {
		timeout = WORK_QUEUE_WAITFORTASK;
	} else {
		timeout = MAX(0, stoptime - time(0));
	}

	struct work_queue_task *t = work_queue_wait(q->work_queue, timeout);
	if(t) {
		info->submitted = t->submit_time / 1000000;
		info->started = t->start_time / 1000000;
		info->finished = t->finish_time / 1000000;
		info->exited_normally = 1;
		info->exit_code = t->return_status;
		info->exit_signal = 0;

		/*
		   If the standard ouput of the job is not empty,
		   then print it, because this is analogous to a Unix
		   job, and would otherwise be lost.  Important for
		   capturing errors from the program.
		 */

		if(t->output && t->output[0]) {
			if(t->output[1] || t->output[0] != '\n') {
				string_chomp(t->output);
				printf("%s\n", t->output);
			}
		}

		char *outfile = itable_remove(q->output_table, t->taskid);
		if(outfile) {
			FILE *file = fopen(outfile, "w");
			if(file) {
				fwrite(t->output, strlen(t->output), 1, file);
				fclose(file);
			}
			free(outfile);
		}
		fprintf(logfile, "TASK %llu %d %d %d %d %llu %llu %llu %llu %llu %s \"%s\" \"%s\"\n", timestamp_get(), t->taskid, t->result, t->return_status, t->worker_selection_algorithm, t->submit_time, t->transfer_start_time, t->finish_time,
			t->total_bytes_transferred, t->total_transfer_time, t->host, t->tag ? t->tag : "", t->command_line);

		taskid = t->taskid;
		work_queue_task_delete(t);
	}
	// Print to work queue log since status has been changed.
	work_queue_get_stats(q->work_queue, &s);
	fprintf(logfile, "QUEUE %llu %d %d %d %d %d %d %d %d %d %d %lld %lld\n", timestamp_get(), s.workers_init, s.workers_ready, s.workers_busy, s.tasks_running, s.tasks_waiting, s.tasks_complete, s.total_tasks_dispatched, s.total_tasks_complete,
		s.total_workers_joined, s.total_workers_removed, s.total_bytes_sent, s.total_bytes_received);
	fflush(logfile);
	fsync(fileno(logfile));

	if(taskid >= 0) {
		return taskid;
	}

	if(work_queue_empty(q->work_queue)) {
		return 0;
	} else {
		return -1;
	}
}

int batch_job_remove_work_queue(struct batch_queue *q, batch_job_id_t jobid)
{
	return 0;
}

