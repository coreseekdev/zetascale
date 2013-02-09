#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "api/fdf.h"
#include "test.h"

static FDF_cguid_t  cguid_shared;
static char *base = "container";
static int iterations = 1000;
static int threads = 1;
static long size = 1024 * 1024 * 1024;

void* worker(void *arg)
{
    int i;

    struct FDF_iterator*		_fdf_iterator;

    FDF_cguid_t  				 cguid;
    char 						 cname[32] 			= "cntr0";
    char        				*data;
    uint64_t     				 datalen;
    char        				*key;
    uint32_t     				 keylen;
    char 						 key_str[24] 		= "key00";
    char 						 key_data[24] 		= "key00_data";

    t(fdf_init_thread(), FDF_SUCCESS);

    sprintf(cname, "%s-%x-mini-trx", base, (int)pthread_self());

    t(fdf_create_container(cname, size, &cguid), FDF_SUCCESS);

    t(fdf_enumerate(cguid, &_fdf_iterator), FDF_SUCCESS);

    while (fdf_next_enumeration(cguid, _fdf_iterator, &key, &keylen, &data, &datalen) == FDF_SUCCESS) {
		fprintf(stderr, "%x sdf_enum: key=%s, keylen=%d, data=%s, datalen=%ld\n", (int)pthread_self(), key, keylen, data, datalen);
		//advance_spinner();
    }

    fprintf(stderr, "\n");

    t(fdf_finish_enumeration(cguid, _fdf_iterator), FDF_SUCCESS);

	t(fdf_transaction_start(), FDF_SUCCESS);

    for(i = 0; i < iterations; i++)
    {
		sprintf(key_str, "key%04ld-%08d", (long) arg, i);
		sprintf(key_data, "key%04ld-%08d_data", (long) arg, i);

		t(fdf_set(cguid, key_str, strlen(key_str) + 1, key_data, strlen(key_data) + 1), FDF_SUCCESS);

		advance_spinner();
    }

	t(fdf_transaction_commit(), FDF_SUCCESS);

    for(i = 0; i < iterations; i++)
    {
		sprintf(key_str, "key%04ld-%08d", (long) arg, i);
		sprintf(key_data, "key%04ld-%08d_data", (long) arg, i);

		t(fdf_get(cguid, key_str, strlen(key_str) + 1, &data, &datalen), FDF_SUCCESS);

		assert(!memcmp(data, key_data, 11));	

		advance_spinner();
    }

    t(fdf_enumerate(cguid, &_fdf_iterator), FDF_SUCCESS);

    while(fdf_next_enumeration(cguid, _fdf_iterator, &key, &keylen, &data, &datalen) == FDF_SUCCESS) {
		fprintf(stderr, "%x sdf_enum: key=%s, keylen=%d, data=%s, datalen=%ld\n", (int)pthread_self(), key, keylen, data, datalen);
		//advance_spinner();
    }

    fprintf(stderr, "\n");

    t(fdf_finish_enumeration(cguid, _fdf_iterator), FDF_SUCCESS);

    return 0;
}

void mini_trx_api_test()
{
	t(fdf_transaction_commit(), FDF_FAILURE_NO_TRANS);

	t(fdf_transaction_start(), FDF_SUCCESS);
	t(fdf_transaction_commit(), FDF_SUCCESS);

	t(fdf_transaction_start(), FDF_SUCCESS);
	t(fdf_transaction_start(), FDF_FAILURE_ALREADY_IN_TRANS);

	t(fdf_transaction_commit(), FDF_SUCCESS);
	t(fdf_transaction_commit(), FDF_FAILURE_NO_TRANS);
}

int main(int argc, char *argv[])
{
    pthread_t thread_id[threads];
    int i;
    char 						 name[32];

	if ( argc < 4 ) {
		fprintf( stderr, "Usage: %s <size in gb> <threads> <iterations>\n", argv[0] );
		return 0;
	} else {
		size = atol( argv[1] ) * 1024 * 1024 * 1024;
		threads = atoi( argv[2] );
		iterations = atoi( argv[3] );
		fprintf(stderr, "size=%lu, hreads=%d, iterations=%d\n", size, threads, iterations);
	}

    t(fdf_init(), FDF_SUCCESS);

    t(fdf_init_thread(), FDF_SUCCESS);

    sprintf(name, "%s-mini-trx", base);

    t(fdf_create_container(name, size, &cguid_shared), FDF_SUCCESS);

	fprintf(stderr, "Mini transaction API tests\n");
	mini_trx_api_test();

	fprintf(stderr, "Multi threaded mini transaction API tests\n");

    for(i = 0; i < threads; i++)
		pthread_create(&thread_id[i], NULL, worker, (void*)(long)i);

    for(i = 0; i < threads; i++)
		pthread_join(thread_id[i], NULL);

    fprintf(stderr, "All tests passed\n");

    return(0);
}
