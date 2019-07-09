#ifndef _AL_YANG_H_
#define _AL_YANG_H_

#include <libyang/libyang.h>

#define YANG_DIR            "/etc/de-agent"
#define YANG_MODULE         "wfa-dataelements"
#define YANG_ROOT_NETWORK   "Network"
#define YANG_ROOT_ASSOC     "AssociationEvent"
#define YANG_ROOT_DISASSOC  "DisassociationEvent"

struct yang_ctx {
	struct ly_ctx *ctx;
	const struct lys_module *mod;
/*
	struct lyd_node *root_network;
	struct lyd_node *root_assoc;
	struct lyd_node *root_disassoc;
*/
};
extern struct yang_ctx *yang_ctx;

/*
 * malloc and initial globe struct yang_ctx;
 * load module wfa-dataelements
 * create root lyd_node
 * @param[in] multiap 1 on multi-ap device, 0 on single-ap device
 * @return -1 on failed, 0 on success
 */
int setup_yang_ctx(int multiap);

/*
 * free globe struct yang_ctx
 * free root lyd_node
 */
void teardown_yang_ctx(void);

/*
 * new or update lyd_node with path and value
 * @param[in] parent Parent lyd_node, such as yang_ctx->root_network
 * @param[in] path Path relative to parent path.
 *       when nodetype LYS_LIST path should be xxx/listname[keyname='keyvalue']
 * @param[in] value Value of the node, value should be "xx,xx,xx" when nodetype is TYS_LEAFLIST
 * @return -1 on failed, 0 on success
 */
int update_lyd_node(struct lyd_node *parent, const char *path, char *value);

/*
 * delete lyd_node with path
 * @param[in] parent Parent lyd_node, such as yang_ctx->root_network
 * param[in] path Path relative to parent path
 * @return -1 on failed, 0 on success
 */
int delete_lyd_node(struct lyd_node *parent, const char *path);

/*
 * search lyd_node with path
 * @param[in] parent Parent lyd_node, such as yang_ctx->root_network
 * @param[in] path Path relative to parent path
 * @param[in] ret_value Value of the return node when nodetype is LYS_LEAF or LYS_LEAFLIST
 *       if not care for node value, let ret_value NULL
 *       if nodetype type is LYS_LEAFLIST, ret_value will be "value[0],value[1],value[2]"
 *       may need free ret_value.
 * @return lyd_node when success, NULL on failed
 */
struct lyd_node *search_lyd_node(struct lyd_node *parent, const char *path, char **ret_value);

#endif

