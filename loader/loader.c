/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "exec_parser.h"
#include <stdbool.h>

static so_exec_t *exec;
static int exec_decriptor;

/* Citirea in buffer a maxim size bytes */
void read_from(char *buffer, int size)
{
	int totalreadBytes = 0;
	while (totalreadBytes < size)
	{
		int readBytes = read(exec_decriptor, buffer + totalreadBytes, size - totalreadBytes);
		if (readBytes == 0)
			fprintf(stderr, "Sfarsitul fisierului!"); /* Am ajuns la sfarsitul fisierului */

		if (readBytes < 0)
			fprintf(stderr, "Eroare citire!"); /* Eroare la citire */

		totalreadBytes += readBytes;
	}
}

/* Copierea instructiunilor la adresa page_address */
void copy_into(so_seg_t *segment, void *page_address, int nr_page)
{
	int page_size = getpagesize();
	int page_offset = page_size * nr_page;
	char *buffer = (char *)malloc(sizeof(char) * page_size);

	lseek(exec_decriptor, segment->offset + page_offset, SEEK_SET);

	if (page_offset > segment->file_size)
	{
		memset(page_address, 0, page_size);
	}
	else if (segment->file_size - page_offset < page_size)
	{
		read_from(buffer, segment->file_size - page_offset);
		memcpy(page_address, buffer, segment->file_size - page_offset);
		memset(page_address + segment->file_size - page_offset, 0, page_size - segment->file_size + page_offset);
	}
	else
	{
		read_from(buffer, page_size);
		memcpy(page_address, buffer, page_size);
	}

	int check = mprotect(page_address, page_size, segment->perm); /* Setare permisiuni segment */
	if (check < 0)
		fprintf(stderr, "Eroare permisiuni!");

	free(buffer);
}

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));

	if (signum != SIGSEGV)
	{
		sa.sa_sigaction(signum, info, context);
		return;
	}

	so_seg_t *segment;
	for (int i = 0; i < exec->segments_no; i++) /* Aflarea segmentului care a produs SEGMENTATION FAULT */
	{
		segment = &exec->segments[i];
		if ((uintptr_t)info->si_addr < (segment->vaddr + segment->mem_size) && (uintptr_t)info->si_addr >= segment->vaddr)
		{
			int nr_page = ((uintptr_t)info->si_addr - segment->vaddr) / getpagesize();

			if (info->si_code == SEGV_MAPERR) /* Verificare mapare pagina */
			{
				void *page_address = mmap((void *)(getpagesize() * nr_page + segment->vaddr), getpagesize(), PERM_R | PERM_W | PERM_X, MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS, -1, 0);
				if (page_address == NULL)
					fprintf(stderr, "Eroare la mapare!");

				copy_into(segment, page_address, nr_page);
				return;
			}
			else
			{
				sa.sa_sigaction(signum, info, context);
				return;
			}
		}
	}
	sa.sa_sigaction(signum, info, context);
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0)
	{
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec_decriptor = open(path, O_RDONLY);

	exec = so_parse_exec(path);
	if (!exec)
	{
		return -1;
	}

	so_start_exec(exec, argv);

	close(exec_decriptor);
	return -1;
}