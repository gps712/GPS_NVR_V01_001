#ifndef __RTP_H__
#define __RTP_H__
#ifdef __cplusplus
extern "C" {
#endif
void SimpleInitRtp(void);
void SimpleUninitRtp(void);
int  SimpleAddDestination(unsigned int ipaddr, unsigned short destport);
int  SimpleH264SendPacket(unsigned char *val,unsigned int length);
int  SimpleDeleteDestination(unsigned int ipaddr,unsigned short  port);
int  SimpleClearDestinations(void);
#ifdef __cplusplus
}
#endif

#endif

