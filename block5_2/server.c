#include <signal.h>
#include <onion/log.h>
#include <onion/onion.h>
#include <onion/block.h>
#include <onion/shortcuts.h>
#include <hashtable/hashtable.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

onion *o = NULL;

onion_connection_status example_handler(void *_, onion_request * req, onion_response * res) {
	int flags = onion_request_get_flags(req);
	int flagextraction = flags & 7;

	const onion_block *dreq = onion_request_get_data(req);

	if (flagextraction == OR_PUT) {
		onion_response_printf(res, "Received put.\n");

		if (dreq) {
			/* libonion stores every put in a tempfile :( */
			char buffer[512];
			FILE *tmpfile;
			size_t nread;

			onion_response_printf(res, "Request Body: ");

			tmpfile = fopen(onion_block_data(dreq), "r");
			if (tmpfile) {
				while ((nread = fread(buffer, 1, 511, tmpfile)) > 0) {
					buffer[nread] = '\0';
					onion_response_printf(res, "%s", buffer);
				}
				fclose(tmpfile);
			}
			onion_response_printf(res, "\n");
		}

		return OCS_PROCESSED;

	} else if (flagextraction == OR_POST) {
		onion_response_printf(res, "Received post.\n");
	} else if (flagextraction == OR_DELETE) {
		onion_response_printf(res, "Received delete.\n");
	} else {
		onion_response_printf(res, "Method not supported!\n");
		return OCS_PROCESSED;
	}

	if (dreq) {
		onion_response_printf(res, "Request Body: %s\n", onion_block_data(dreq));
	}

	return OCS_PROCESSED;
}

onion_connection_status movie_handler(void *_, onion_request * req, onion_response * res) {
	int flags = onion_request_get_flags(req);
	int flagextraction = flags & 7;

	const onion_block *dreq = onion_request_get_data(req); // Request body
	const char *rqpath = onion_request_get_path(req); // Request query

	unsigned int key = 0;

	if (flagextraction == OR_GET) {
		printf("%s", "Received GET.\n");
		if (rqpath) {
			key = ht_hash(rqpath, strlen(rqpath));
		}

		printf("Key: %s", &key);
		struct element *e;
		if ((e = ht_get((char *) &key, sizeof(key))) != NULL) {
			printf("Val: %s", e->value);
			onion_response_printf(res, "%s", e->value);
		}
	}
	else if (flagextraction == OR_POST) {
		// if path, deny

		if (dreq) {
			key = ht_hash(onion_block_data(dreq), strlen(onion_block_data(dreq)));
		}

		ht_set((char *) &key, (char *) onion_block_data(dreq), sizeof(key), sizeof(onion_block_data(dreq)));
		onion_response_printf(res, "{\"id\":%u}\n", key);
	} else if (flagextraction == OR_DELETE) {
		if (rqpath) {
			key = ht_hash(rqpath, strlen(rqpath));
		}

		ht_del((char *) &key, sizeof(key));
		onion_response_printf(res, "Received delete.\n");
	} else {
		onion_response_printf(res, "Method not supported!\n");
		return OCS_PROCESSED;
	}

	if (rqpath) {
		onion_response_printf(res, "Request Path: %s\n", rqpath);
	}

	if (dreq) {
		onion_response_printf(res, "Request Body: %s\n", onion_block_data(dreq));
	}

	return OCS_PROCESSED;
}

void onexit(int _) {
	ONION_INFO("Exit");
	if (o) {
		onion_listen_stop(o);
	}
}

int main(int argc, char **argv) {

	o = onion_new(O_ONE_LOOP);
	onion_set_port(o, "8080");
	onion_url *urls = onion_root_url(o);

	ht_init(100);

	onion_url_add_static(urls, "", "Server running :)!\n", HTTP_OK);

	onion_url_add(urls, "example", example_handler);
	onion_url_add(urls, "^movie/", movie_handler);

	signal(SIGTERM, onexit);
	signal(SIGINT, onexit);
	onion_listen(o);

	onion_free(o);
	return 0;
}
