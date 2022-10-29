#include "status.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

const int agent[14] = {-1, 14, -1, 10, 13, -1, 8, 11, 9, 12, -1, -1, -1, -1};
const char first_battle[] = "GGHHIIJJMMKNNLC";

FILE *flog;

int player_id;
int parent_pid;

Status status;

static inline void rp (int fd)
{
	char buf[87];
	read(fd, buf, 87);
	memcpy(&status, buf, sizeof(Status));
	fprintf(flog, "%d,%d pipe from %c,%d %d,%d,%c,%d\n", player_id, getpid(), first_battle[player_id], parent_pid, status.real_player_id, status.HP, status.current_battle_id, status.battle_ended_flag);
	fflush(flog);
}

static inline void wp (int fd)
{
	fprintf(flog, "%d,%d pipe to %c,%d %d,%d,%c,%d\n", player_id, getpid(), first_battle[player_id], parent_pid, status.real_player_id, status.HP, status.current_battle_id, status.battle_ended_flag);
	fflush(flog);
	write(fd, &status, sizeof(Status));
}

static inline void rf (int fd)
{
	char buf[87];
	read(fd, buf, 87);
	memcpy(&status, buf, sizeof(Status));
	sprintf(buf, "./log_player%d.txt", status.real_player_id);
	flog = fopen(buf, "a");
	fprintf(flog, "%d,%d fifo from %d %d,%d\n", player_id, getpid(), status.real_player_id, status.real_player_id, status.HP);
	fflush(flog);
}

static inline void wf (int fd)
{
	fprintf(flog, "%d,%d fifo to %d %d,%d\n", player_id, getpid(), agent[status.current_battle_id - 'A'], player_id, status.HP);
	fflush(flog);
	write(fd, &status, sizeof(Status));
}

int main (int argc, char *argv[])
{
	player_id = atoi(argv[1]);
	parent_pid = atoi(argv[2]);

	int FHP;

	if (player_id <= 7)
	{
		FILE *orig_status = fopen("./player_status.txt", "r");
		for (int i = 0; i <= player_id; i++)
		{
			char buf[100], bbuf[10];
			fgets(buf, 100, orig_status);
			sscanf(buf, "%d%d%s %c%d", &status.HP, &status.ATK, bbuf, &status.current_battle_id, &status.battle_ended_flag);
			switch (*bbuf)
			{
				case 'F':
					status.attr = FIRE;
					break;
				case 'G':
					status.attr = GRASS;
					break;
				case 'W':
					status.attr = WATER;
			}
		}
		fclose(orig_status);
		status.real_player_id = player_id;
		char log_name[99];
		sprintf(log_name, "./log_player%d.txt", player_id);
		flog = fopen(log_name, "a");
	}
	else
	{
		char fifo_name[88];
		sprintf(fifo_name, "./player%d.fifo", player_id);
		mkfifo(fifo_name, 0600);
		int fifofd = open(fifo_name, O_RDONLY);
		rf(fifofd);
	}

	FHP = status.HP;

	while (true)
	{
		status.battle_ended_flag = 0;
		while (!status.battle_ended_flag)
		{
			wp(1);
			rp(0);
		}

		if (status.current_battle_id == 'A')
		{
			exit(0);
		}

		if (status.HP > 0)
		{
			status.HP = (FHP + status.HP) / 2;
		}
		else
		{
			status.HP = FHP;
			if (player_id <= 7)
			{
				char fifo_name[38];
				sprintf(fifo_name, "./player%d.fifo", agent[status.current_battle_id - 'A']);
				int fifofd;
				while ((fifofd = open(fifo_name, O_WRONLY)) < 0);
				wf(fifofd);
			}
			exit(0);
		}
	}

	return 0;
}