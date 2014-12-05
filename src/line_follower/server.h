/* TODO HEADER COMMENT, LICENSE STUFF ETC */
#define PACKET_LEN (5)

/* TODO FUNCTION COMMENTS */
int lf_init(char * path);

int lf_close(void);

int lf_send(char * p_data, int len);

int lf_receive(char * p_data, int * got_data);

