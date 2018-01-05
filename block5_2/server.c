#include <signal.h>
#include <onion/log.h>
#include <onion/onion.h>
#include <onion/block.h>
#include <onion/shortcuts.h>
#include <onion/types.h>
#include <hashtable/hashtable.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

onion *o = NULL;
onion_connection_status movie_handler(void *_, onion_request * req, onion_response * res) {
	int flags = onion_request_get_flags(req);
	int flagextraction = flags & 7;

	const onion_block *dreq = onion_request_get_data(req); // Request body
	const char *rqpath = onion_request_get_path(req); // Request query
	char key[16];

	if (flagextraction == OR_GET) {
		if (rqpath && !rqpath[0]) {
			onion_response_set_code(res, HTTP_BAD_REQUEST);
			return OCS_PROCESSED;
		}

		struct element *e;
		if ((e = ht_get((char*)rqpath, strlen(rqpath))) == NULL) {
			onion_response_set_code(res, HTTP_NOT_FOUND);
			return OCS_PROCESSED;
		}

		onion_response_printf(res, "%s\n", e->value);
	}

	else if (flagextraction == OR_POST) {
		if (!dreq || (rqpath && rqpath[0])) {
			onion_response_set_code(res, HTTP_BAD_REQUEST);
			return OCS_PROCESSED;
		}

		char *reqbody = (char*) onion_block_data(dreq);
		sprintf(key, "%u", ht_hash(reqbody, strlen(reqbody)));

		if (ht_set(key, reqbody, strlen(key), strlen(reqbody)) != 1) {
			onion_response_set_code(res, HTTP_INTERNAL_ERROR);
			return OCS_PROCESSED;
		}

		onion_response_set_code(res, HTTP_CREATED);
		onion_response_printf(res, "{\"id\":%s}\n", key);

	} else if (flagextraction == OR_DELETE) {
		if (rqpath && !rqpath[0]) {
			onion_response_set_code(res, HTTP_BAD_REQUEST);
			return OCS_PROCESSED;
		}

		if (ht_del((char*)rqpath, strlen(rqpath)) != 1) {
			onion_response_set_code(res, HTTP_INTERNAL_ERROR);
			return OCS_PROCESSED;
		};

		onion_response_set_code(res, 204);
	} else {
		onion_response_printf(res, "Method not supported!\n");
	}

	return OCS_PROCESSED;
}

void onexit(int _) {
	ONION_INFO("Exit");
	if (o) {
		onion_listen_stop(o);
	}
	cleanup();
}

int main(int argc, char **argv) {
	if(argc != 2) {
		fprintf(stderr, "arguments: port\n");
		return 1;
	}

	o = onion_new(O_ONE_LOOP);
	onion_set_port(o, argv[1]);
	onion_url *urls = onion_root_url(o);

	ht_init(100);

	onion_url_add_static(urls, "", "Server running :)!\n", HTTP_OK);
	onion_url_add(urls, "^movie/", movie_handler);

	signal(SIGTERM, onexit);
	signal(SIGINT, onexit);
	onion_listen(o);

	onion_free(o);
	return 0;
}
