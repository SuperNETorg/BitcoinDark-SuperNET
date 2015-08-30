/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#define BUNDLED
#define PLUGINSTR "dcnet"
#define PLUGNAME(NAME) dcnet ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../plugin777.c"
#undef DEFINES_ONLY

union _bits256 { uint8_t bytes[32]; uint16_t ushorts[16]; uint32_t uints[8]; uint64_t ulongs[4]; uint64_t txid; };
typedef union _bits256 bits256;
union _bits320 { uint8_t bytes[40]; uint16_t ushorts[20]; uint32_t uints[10]; uint64_t ulongs[5]; uint64_t txid; };
typedef union _bits320 bits320;
bits320 Unit;
int32_t init_hexbytes_noT(char *hexbytes,uint8_t *message,long len);
int32_t safecopy(char *dest,char *src,long len);
char *clonestr(char *str);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);

#include "../../utils/curve25519.h"

bits256 rand256(int32_t privkeyflag)
{
    bits256 randval;
    randombytes(randval.bytes,sizeof(randval));
    if ( privkeyflag != 0 )
        randval.bytes[0] &= 0xf8, randval.bytes[31] &= 0x7f, randval.bytes[31] |= 0x40;
    return(randval);
}

bits256 curve25519_jl777(bits256 secret,const bits256 basepoint)
{
    bits256 curve25519(bits256 secret,const bits256 basepoint);
    bits320 bp,x,z; bits256 check,pubkey; int32_t i;
    secret.bytes[0] &= 0xf8, secret.bytes[31] &= 0x7f, secret.bytes[31] |= 64;
    bp = fexpand(basepoint);
    cmult(&x,&z,secret,bp);
    z = fmul(x,crecip(z)); // x/z -> pubkey, secret * 9 -> (x,z)
    check = curve25519(secret,basepoint);
    pubkey = fcontract(z);
    if ( memcmp(check.bytes,pubkey.bytes,sizeof(check)) != 0 )
    {
        for (i=0; i<4; i++)
            printf("%016llx ",(long long)check.ulongs[i]);
        printf("check\n");
        for (i=0; i<4; i++)
            printf("%016llx ",(long long)pubkey.ulongs[i] ^ check.ulongs[i]);
        printf("pubkey\n");
        printf("compare error on pubkey %llx vs %llx\n",(long long)check.txid,(long long)pubkey.txid);
    }
    return(pubkey);
}

bits256 keypair(bits256 *pubkeyp)
{
    bits256 basepoint,privkey;
    memset(&basepoint,0,sizeof(basepoint));
    basepoint.bytes[0] = 9;
    privkey = rand256(1);
    *pubkeyp = curve25519_jl777(privkey,basepoint);
    return(privkey);
}

struct dcnode
{
    bits320 pubexp,pubexp2;
};

STRUCTNAME
{
    int32_t bus,mode,num;
    char bind[128],connect[128];
    bits256 privkey,privkey2;
    bits320 pubexp,pubexp2;
    struct dcnode group[64];
} DCNET;

void dcinit()
{
    bits256 pubkey,pubkey2;
    DCNET.privkey = keypair(&pubkey), DCNET.privkey2 = keypair(&pubkey2);
    DCNET.pubexp = fexpand(pubkey), DCNET.pubexp2 = fexpand(pubkey2);
}

void set_dcnode(struct dcnode *node,bits320 pubexp,bits320 pubexp2)
{
    node->pubexp = pubexp, node->pubexp2 = pubexp2;
}

bits320 dcround(bits320 *commitp,int32_t i,struct dcnode *nodes,int32_t n,bits256 G,bits256 H)
{
    int32_t j,senderi = 0; bits320 prod,commit,c,x,z,x2,z2,shared,shared2;
    prod = (i != senderi) ? Unit : fexpand(rand256(1));
    commit = Unit;
    for (j=0; j<n; j++)
    {
        memset(c.bytes,0,sizeof(c));
        if ( i != j )
        {
            cmult(&x,&z,DCNET.privkey,nodes[j].pubexp);
            cmult(&x2,&z2,DCNET.privkey2,nodes[j].pubexp2);
            if ( i < j )
                shared = fmul(x,crecip(z)), shared2 = fmul(x2,crecip(z2));
            else shared = fmul(z,crecip(x)), shared2 = fmul(z2,crecip(x2));
            cmult(&x,&z,G,shared);
            cmult(&x2,&z2,H,shared2);
            if ( i < j )
                c = fmul(fmul(x,crecip(z)),fmul(x2,crecip(z2)));
            else c = fmul(fmul(z,crecip(x)),fmul(z2,crecip(x2)));
            commit = fmul(commit,c);
            prod = fmul(shared,prod);
        }
        if ( i == 0 && j == 0 )
            printf("%016llx ",prod.txid);
        else printf("%016llx ",c.txid);
    }
    *commitp = commit;
    return(prod);
}

/*{
 for (i=0; i<numcards; i++)
 init_card(&cards[i],i);
 if ( Unit.txid == 0 )
 Unit = fmul(cards[0].z,cards[0].zmone);
 G = rand256(1);
 H = rand256(1);
 total = Unit;
 totalsum = Unit;
 for (i=0; i<numcards; i++)
 {
 prod = dcround(&commit,i,cards,numcards,G,H);
 total = fmul(total,prod);
 totalsum = fmul(totalsum,commit);
 printf("-> %016llx commit %016llx\n",prod.txid,commit.txid);//,total.txid,totalsum.txid);
 //printf("%llx ",total.txid);
 }
 printf("-> %16llx totalsum %llx G.%llx H.%llx\n",total.txid,totalsum.txid,G.txid,H.txid);
 }*/

void dcnet(char *dccmd,char *pubkeystr,char *pubkey2str)
{
    printf("dcnet.(%s) P.(%s) P2.(%s)\n",dccmd,pubkeystr,pubkey2str);
    int32_t i,j; bits256 pubkey,pubkey2; bits320 pubexp,pubexp2;
    decode_hex(pubkey.bytes,sizeof(pubkey),pubkeystr), pubexp = fexpand(pubkey);
    decode_hex(pubkey2.bytes,sizeof(pubkey2),pubkey2str), pubexp2 = fexpand(pubkey2);
    if ( strcmp(dccmd,"join") == 0 )
    {
        for (i=0; i<DCNET.num; i++)
        {
            if ( pubexp.txid == DCNET.group[i].pubexp.txid )
            {
                printf("duplicate %llx, DCNET.num %d\n",(long long)pubexp.txid,DCNET.num);
                return;
            }
            if ( pubexp.txid > DCNET.group[i].pubexp.txid )
            {
                for (j=DCNET.num; j>i; j--)
                    DCNET.group[j] = DCNET.group[j-1];
                break;
            }
        }
        set_dcnode(&DCNET.group[i],pubexp,pubexp2);
        DCNET.num++;
        for (i=0; i<DCNET.num; i++)
            printf("%llx ",(long long)DCNET.group[i].pubexp.txid);
        printf("DCNET.num %d\n",DCNET.num);
    }
}

int32_t dcnet_idle(struct plugin_info *plugin)
{
    int32_t len; char *msg,*jsonstr,*pubkey,*pubkey2,*dccmd; cJSON *json;
    if ( DCNET.bus >= 0 )
    {
        if ( (len= nn_recv(DCNET.bus,&msg,NN_MSG,0)) > 0 )
        {
            jsonstr = clonestr(msg);
            nn_freemsg(msg);
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                if ( (dccmd= jstr(json,"dcnet")) != 0 )
                {
                    pubkey = jstr(json,"pubkey"), pubkey2 = jstr(json,"pubkey2");
                    if ( pubkey != 0 && pubkey2 != 0 )
                        dcnet(dccmd,pubkey,pubkey2);
                }
                free_json(json);
            }
            free(jsonstr);
        }
    }
    return(0);
}

#define DCNET_API "join"
char *PLUGNAME(_methods)[] = { DCNET_API };
char *PLUGNAME(_pubmethods)[] = { DCNET_API };
char *PLUGNAME(_authmethods)[] = { "" };

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    plugin->allowremote = 1;
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char connectaddr[64],*resultstr,*methodstr,*myip,*retstr = 0; bits256 tmp; bits320 z,zmone; int32_t i,len,sendtimeout = 10,recvtimeout = 1;
    retbuf[0] = 0;
    plugin->allowremote = 1;
    if ( initflag > 0 )
    {
        char *ipaddrs[] = { "5.9.56.103", "5.9.102.210", "89.248.160.237", "89.248.160.238", "89.248.160.239", "89.248.160.240", "89.248.160.241", "89.248.160.242" };
        fprintf(stderr,"<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
        for (i=0; i<1000; i++)
        {
            tmp = rand256(1);
            curve25519_jl777(tmp,rand256(0));
        }
        z = fexpand(tmp);
        zmone = crecip(z);
        Unit = fmul(z,zmone);
        dcinit();
        DCNET.bus = -1;
        DCNET.mode = juint(json,"dchost");
        myip = jstr(json,"myipaddr");
        if ( DCNET.mode != 0 )
        {
            if ( DCNET.mode > 1 && myip != 0 )
            {
                safecopy(plugin->ipaddr,myip,sizeof(plugin->ipaddr));
                if ( (DCNET.bus= nn_socket(AF_SP,NN_BUS)) != 0 )
                {
                    sprintf(DCNET.bind,"tcp://%s:9999",myip);
                    if ( nn_bind(DCNET.bus,DCNET.bind) >= 0 )
                    {
                        if ( nn_settimeouts2(DCNET.bus,sendtimeout,recvtimeout) != 0 )
                            printf("error setting timeouts\n");
                        else printf("DCBIND.(%s) -> NN_BUS\n",DCNET.bind);
                    } else printf("error with DCBIND.(%s)\n",DCNET.bind);
                }
            } else strcpy(plugin->ipaddr,"127.0.0.1");
            if ( DCNET.bus < 0 && (DCNET.bus= nn_socket(AF_SP,NN_BUS)) != 0 )
            {
                if ( nn_settimeouts2(DCNET.bus,sendtimeout,recvtimeout) != 0 )
                    printf("error setting timeouts\n");
                for (i=0; i<sizeof(ipaddrs)/sizeof(*ipaddrs); i++)
                {
                    if ( strcmp(ipaddrs[i],myip) != 0 )
                    {
                        sprintf(connectaddr,"tcp://%s:9999",ipaddrs[i]);
                        if ( nn_connect(DCNET.bus,connectaddr) >= 0 )
                            printf("+%s ",connectaddr);
                    }
                }
            }
        }
        printf("ipaddr.%s bindaddr.%s\n",plugin->ipaddr,DCNET.bind);
        strcpy(retbuf,"{\"result\":\"dcnet init\"}");
    }
    else
    {
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        retbuf[0] = 0;
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
            return((int32_t)strlen(retbuf));
        }
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        else if ( strcmp(methodstr,"join") == 0 )
        {
            bits256 pubkey,pubkey2; char pubhex[65],pubhex2[65];
            pubkey = fcontract(DCNET.pubexp), pubkey2 = fcontract(DCNET.pubexp2);
            init_hexbytes_noT(pubhex,pubkey.bytes,sizeof(pubkey));
            init_hexbytes_noT(pubhex2,pubkey2.bytes,sizeof(pubkey2));
            sprintf(retbuf,"{\"result\":\"success\",\"dcnet\":\"join\",\"pubkey\":\"%s\",\"pubkey2\":\"%s\",\"done\":1,\"bind\":\"%s\",\"connect\":\"%s\"}",pubhex,pubhex2,DCNET.bind,DCNET.connect);
            len = (int32_t)strlen(retbuf);
            if ( DCNET.mode > 0 && DCNET.bus >= 0 )
            {
                printf("DCSEND.(%s)\n",retbuf);
                nn_send(DCNET.bus,retbuf,len,0);
            }
            return(len);
        }
    }
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../plugin777.c"
