#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "yang.h"

struct yang_ctx *yang_ctx = NULL;

int setup_yang_ctx(int multiap)
{
	yang_ctx = malloc(sizeof(struct yang_ctx));
	if (!yang_ctx) {
		printf("Memory allocation error");
		return -1;
	}

	yang_ctx->ctx = ly_ctx_new(YANG_DIR);
	if (!yang_ctx->ctx) {
		printf("Failed to create context.\n");
		goto error;
	}

	yang_ctx->mod = ly_ctx_load_module(yang_ctx->ctx, YANG_MODULE, NULL);
	if (!yang_ctx->mod) {
		printf("Failed to load yang module \"%s\".\n", YANG_MODULE);
		goto error;	
	}

	if (multiap)
		lys_features_enable(yang_ctx->mod, "multi-ap");
	else
		lys_features_disable(yang_ctx->mod, "multi-ap");
/*
	yang_ctx->root_network = lyd_new(NULL, yang_ctx->mod, YANG_ROOT_NETWORK);
	yang_ctx->root_assoc = lyd_new(NULL, yang_ctx->mod, YANG_ROOT_ASSOC);
	yang_ctx->root_disassoc = lyd_new(NULL, yang_ctx->mod, YANG_ROOT_DISASSOC);
*/
	return 0;

error:
	ly_ctx_destroy(yang_ctx->ctx, NULL);
	free(yang_ctx);
	yang_ctx = NULL;
	return -1;
}

void teardown_yang_ctx(void)
{
	if (yang_ctx->ctx)
		ly_ctx_destroy(yang_ctx->ctx, NULL);
/*
	if (yang_ctx->root_network)
		lyd_free(yang_ctx->root_network);
	if (yang_ctx->root_assoc)
		lyd_free(yang_ctx->root_assoc);
	if (yang_ctx->root_disassoc)
		lyd_free(yang_ctx->root_disassoc);
*/
	if (yang_ctx)
		free(yang_ctx);

	yang_ctx = NULL;
}

int update_lyd_node(struct lyd_node *parent, const char *path, char *value)
{
	const char *subpath = NULL;
	char *subvalue = NULL;
	char *values = NULL;
	char child_name[56] = {0};
	char *child_path = NULL;
	struct ly_set *set = NULL;
	struct lyd_node *child_lyd = NULL;
	const struct lys_node *child_lys = NULL;
	int i = 0;
	char leaflist_name[64] = {0};

	if (NULL == parent || NULL == path || 0 == *path)
		return -1;

	if ('/' == *path)
		path++;
	subpath = strchr(path, '/');
	strncpy(child_name, path, subpath != NULL? (subpath-path) : strlen(path));

	while (NULL != (child_lys = lys_getnext(child_lys, parent->schema,
				yang_ctx->mod, LYS_GETNEXT_WITHGROUPING))) {
		if (strncmp(child_name, child_lys->name, strlen(child_lys->name)))
			continue;

		if (lys_is_disabled(child_lys, 0))
			continue;

		set = lyd_get_node(parent, child_lys->name);
		if (set != NULL && set->number > 0) {
			for (i = 0; i < set->number; i++) {
				child_path = strrchr(lyd_path(set->set.d[i]), '/');
				if (child_path)
					child_path++;
				if (NULL != child_path && 0 == strcmp( child_name, child_path))
					child_lyd = set->set.d[i];
			}
		}

	        switch (child_lys->nodetype) {
		case LYS_CONTAINER:
			if (NULL == child_lyd) {
				child_lyd = lyd_new(parent, yang_ctx->mod, child_name);
				if (NULL == child_lyd) {
					printf("new container lyd_node %s/%s failed\n", lyd_path(parent), child_name);
					goto fail;
				}
			}
			if (subpath && update_lyd_node(child_lyd, subpath, value))
				goto fail;
			else
				goto success;

		case LYS_LIST:
			if (NULL == child_lyd) {
				child_lyd = lyd_new_path(parent, yang_ctx->ctx, child_name, NULL, 0);
				if (NULL == child_lyd)
					child_lyd = lyd_new(parent, yang_ctx->mod, child_name);
				if (NULL == child_lyd) {
					printf("new list lyd_node %s/%s failed\n", lyd_path(parent), child_name);
					goto fail;
				}
			}
			if (subpath && update_lyd_node(child_lyd, subpath, value))
				goto fail;
			else
				goto success;

		case LYS_LEAF:
			if (NULL == child_lyd) {
				child_lyd = lyd_new_leaf(parent, yang_ctx->mod, child_name, value);
				if (NULL == child_lyd) {
					printf("new leaf lyd_node %s/%s failed\n", lyd_path(parent), child_name);
					goto fail;
				}
			} else {
				if (lyd_change_leaf((struct lyd_node_leaf_list *)child_lyd, value)) {
					printf("change leaf lyd_node %s/%s value %s failed\n",
							lyd_path(parent), child_name, value);
					goto fail;
				}
			}
			goto success;

		case LYS_LEAFLIST:
			if (set != NULL && set->number > 0) {
				for (i = 0; i < set->number; i++)
					lyd_free(set->set.d[i]);
			}
			if (value) {
				values = strdup(value);
				subvalue = strtok(values, ",");
				while (subvalue) {
					snprintf(leaflist_name, sizeof(leaflist_name)-1, "%s[.='%s']",
							child_name, subvalue);
					child_lyd = lyd_new_path(parent, yang_ctx->ctx, leaflist_name, NULL, 0);
					if ( NULL == child_lyd) {
						printf("new leaf-list lyd_node %s/%s failed\n",
								lyd_path(parent), leaflist_name);
						free(values);
						goto fail;
					}
					subvalue = strtok(NULL, ",");
				}
				free(values);
				goto success;
			} else {
				child_lyd = lyd_new_leaf(parent, yang_ctx->mod, child_name, NULL);
				if (NULL == child_name) {
					printf("new leaf-list lyd_node %s/%s failed\n",
							lyd_path(parent), child_name);
					goto fail;
				} else
					goto success;
			}

		default:
			continue;
		}
	}
	printf("cannot find path %s/%s in the schema\n", lyd_path(parent), child_name);

fail:
	ly_set_free(set);
	return -1;

success:
	ly_set_free(set);
	return 0;
}

int delete_lyd_node(struct lyd_node *parent, const char *path)
{
	struct ly_set *set = NULL;
	int i = 0;

	if (NULL == parent || NULL == path || 0 == *path)
		return -1;

	set = lyd_get_node(parent, path);
	if (set != NULL) {
		for (i = 0; i < set->number; i++)
			lyd_free(set->set.d[i]);
	}
	ly_set_free(set);

	return 0;
}

struct lyd_node *search_lyd_node(struct lyd_node *parent, const char *path, char **ret_value)
{
	struct lyd_node *node = NULL;
	struct ly_set *set = NULL;
	int i = 0;
	char value_str[128] = {0};

	if (NULL == parent || NULL == path || 0 == *path)
		return NULL;

	set = lyd_get_node(parent, path);
	if (set != NULL && set->number > 0) {
		node = set->set.d[0];
		if (ret_value != NULL) {
			if (node->schema->nodetype == LYS_LEAF)
				*ret_value = strdup(((struct lyd_node_leaf_list *)node)->value_str);
			else if (node->schema->nodetype == LYS_LEAFLIST) {
				snprintf(value_str, sizeof(value_str)-1, "%s", ((struct lyd_node_leaf_list *)node)->value_str);
				printf("value_str = %s\n", value_str);
				for (i = 1; i < set->number; i++)
					snprintf(value_str,  sizeof(value_str)-1, "%s,%s", value_str,
							((struct lyd_node_leaf_list *)(set->set.d[i]))->value_str);
				*ret_value = strdup(value_str);
			}
		}
		return node;
	} else
		return NULL;
}
