#ifndef _video_h_
#define _video_h_


typedef enum {
    ENCODER_UNKNOWN,
    ENCODER_CONEXANT,
    ENCODER_FOCUS,
    ENCODER_XCALIBUR
} encoder_type;

encoder_type video_get_encoder_type(void);

void video_off(void);
void video_blank(void);
void video_keep(void);
int video_add_avsave_param(char *param, unsigned long param_size);


#endif /* _video_h_ */
