

/*
 * CloudVPN
 *
 * This program is a free software: You can redistribute and/or modify it
 * under the terms of GNU GPLv3 license, or any later version of the license.
 * The program is distributed in a good hope it will be useful, but without
 * any warranty - see the aforementioned license for more details.
 * You should have received a copy of the license along with this program;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#include "mutex.h"

/*
 * this wraps pthread mutexes. Feel free to supply anything better.
 */

#include "alloc.h"
#include <pthread.h>

int cl_mutex_init(cl_mutex* mp)
{
	*mp=cl_malloc(sizeof(pthread_mutex_t));
	if(!(*mp)) return 1;
	return pthread_mutex_init((pthread_mutex_t*) *mp, 0);
}

int cl_mutex_destroy(cl_mutex m)
{
	int r=pthread_mutex_destroy((pthread_mutex_t*)m);
	cl_free(m);
	return r;
}

int cl_mutex_lock(cl_mutex m)
{
	return pthread_mutex_lock((pthread_mutex_t*)m);
}

int cl_mutex_trylock(cl_mutex m)
{
	return pthread_mutex_trylock((pthread_mutex_t*)m);
}

int cl_mutex_unlock(cl_mutex m)
{
	return pthread_mutex_unlock((pthread_mutex_t*)m);
}
