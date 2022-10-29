#include "status.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

const char child[14][2] = {"BC", "DE", 'F', 14, "GH", "IJ", "KL", 0, 1, 2, 3, 4, 5, 6, 7, 'M', 10, 'N', 13, 8, 9, 11, 12};
const Attribute field[14] = {FIRE, GRASS, WATER, WATER, FIRE, FIRE, FIRE, GRASS, WATER, GRASS, GRASS, GRASS, FIRE, WATER};
const char parent[14] = "XAABBCDDEEFFKL";

FILE *flog;

char battle_id;
int parent_pid;

static inline void r (int fd, Status *a, int player_id, int player_pid, bool is_player)
{
	char buf[95];
	read(fd, buf, 95);
	memcpy(a, buf, sizeof(Status));
	fprintf(flog, is_player ? "%c,%d pipe from %d,%d %d,%d,%c,%d\n" : "%c,%d pipe from %c,%d %d,%d,%c,%d\n", battle_id, getpid(), player_id, player_pid, a->real_player_id, a->HP, a->current_battle_id, a->battle_ended_flag);
	fflush(flog);
}

static inline void w (int fd, Status *a, int player_id, int player_pid, bool is_player)
{
	fprintf(flog, is_player ? "%c,%d pipe to %d,%d %d,%d,%c,%d\n" : "%c,%d pipe to %c,%d %d,%d,%c,%d\n", battle_id, getpid(), player_id, player_pid, a->real_player_id, a->HP, a->current_battle_id, a->battle_ended_flag);
	fflush(flog);
	write(fd, a, sizeof(Status));
}

static inline void a (Status *attacker, Status *defender)
{
	int atk = attacker->ATK << (attacker->attr == field[battle_id - 'A']);
	defender->HP -= atk;
	if (defender->HP <= 0)
	{
		attacker->battle_ended_flag = defender->battle_ended_flag = 1;
	}
}

int main (int argc, char *argv[])
{
	battle_id = *argv[1];
	parent_pid = atoi(argv[2]);

	char log_name[87];
	sprintf(log_name, "./log_battle%c.txt", battle_id);
	flog = fopen(log_name, "a");

	const char *child_id = child[battle_id - 'A'];
	int child_pid[2];
	int read_pipe[2];
	int write_pipe[2];

	int pipefd[2][2];

	char pid[92];
	sprintf(pid, "%d", getpid());

	for (int i = 0; i < 2; i++)
	{
		pipe(pipefd[0]), pipe(pipefd[1]);
		child_pid[i] = fork();
		if (!child_pid[i])
		{
			dup2(pipefd[0][1], 1);
			dup2(pipefd[1][0], 0);
			for (int j = 0; j < 4; j++) close(*((int *) pipefd + j));
			if (child_id[i] < 'A')
			{
				char bb[77];
				sprintf(bb, "%d", child_id[i]);
				execl("./player", "./player", bb, pid, NULL);
			}
			else
			{
				execl("./battle", "./battle", (char [2]) {child_id[i], '\0'}, pid, NULL);
			}
		}

		read_pipe[i] = pipefd[0][0];
		write_pipe[i] = pipefd[1][1];
		close(pipefd[0][1]), close(pipefd[1][0]);
	}

	Status status[2] = {};
	while (!status[0].battle_ended_flag)
	{
		for (int i = 0; i < 2; i++) r(read_pipe[i], &status[i], child_id[i], child_pid[i], child_id[i] < 'A'), status[i].current_battle_id = battle_id;
		int attacker = status[0].HP > status[1].HP || status[0].HP == status[1].HP && status[0].real_player_id > status[1].real_player_id;
		a(&status[attacker], &status[attacker ^ 1]);
		if (status[attacker ^ 1].HP > 0) attacker ^= 1, a(&status[attacker], &status[attacker ^ 1]);
		for (int i = 0; i < 2; i++) w(write_pipe[i], &status[i], child_id[i], child_pid[i], child_id[i] < 'A');
	}

	wait(0);

	int winner = status[1].HP > 0;

	if (battle_id == 'A')
	{
		wait(0);
		printf("Champion is P%d\n", status[winner].real_player_id);
		fflush(stdout);
		exit(0);
	}

	while (status[winner].HP > 0 && status[winner].current_battle_id != 'A')
	{
		do
		{
			r(read_pipe[winner], &status[winner], child_id[winner], child_pid[winner], child_id[winner] < 'A');
			w(1, &status[winner], parent[battle_id - 'A'], parent_pid, 0);
			r(0, &status[winner], parent[battle_id - 'A'], parent_pid, 0);
			w(write_pipe[winner], &status[winner], child_id[winner], child_pid[winner], child_id[winner] < 'A');
		} while (!status[winner].battle_ended_flag);
	}

	wait(0);
	return 0;
}