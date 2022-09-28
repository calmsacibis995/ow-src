/* rpcextras.h */
/* stuff they should have included in <rpc/rpc.h> */

struct rpcgen_table {
    char	*(*proc)();
    xdrproc_t	xdr_arg;
    unsigned	len_arg;
    xdrproc_t	xdr_res;
    unsigned	len_res;
};

/* you might want to consider a program list rather than a table */
/* a list would be cleaner, a table easier.  it's a table here for clarity */
typedef struct prog_table {
	struct rpcgen_table *vers;
	u_long nproc;
	} program_table;

typedef struct prog_object {
	program_table *prog;
	u_long nvers;
	u_long program_num;
	} program_object;

typedef program_object *program_handle;

extern program_handle newph();
extern program_handle getph();
