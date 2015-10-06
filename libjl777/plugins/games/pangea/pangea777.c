/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#define BUNDLED
#define PLUGINSTR "pangea"
#define PLUGNAME(NAME) pangea ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
//#include "../../utils/huffstream.c"
#include "../../common/hostnet777.c"
#include "../../includes/cJSON.h"
#include "../../agents/plugin777.c"
#undef DEFINES_ONLY

#include "../../includes/InstantDEX_quote.h"
#include "../../utils/curve25519.h"

#define _PANGEA_MAXTHREADS 9
#define PANGEA_MINRAKE_MILLIS 5
#define PANGEA_USERTIMEOUT 60
#define PANGEA_MAX_HOSTRAKE 10

struct pangea_info
{
    uint32_t timestamp,numaddrs,minbuyin,maxbuyin; uint64_t basebits,bigblind,ante,addrs[CARDS777_MAXPLAYERS],tableid; char base[16]; int32_t myind;
    struct pangea_thread *tp; struct cards777_privdata *priv; struct cards777_pubdata *dp;
} *TABLES[100];

struct pangea_thread
{
    union hostnet777 hn; uint64_t nxt64bits; int32_t threadid,ishost,M,N,numcards;
} *THREADS[_PANGEA_MAXTHREADS];

int32_t PANGEA_MAXTHREADS = _PANGEA_MAXTHREADS;
int32_t Showmode=1,Autofold;
//uint64_t Pangea_waiting,Pangea_userinput_betsize; uint32_t Pangea_userinput_starttime; int32_t Pangea_userinput_cardi; char Pangea_userinput[128];

char *clonestr(char *);
uint64_t stringbits(char *str);

uint32_t set_handstr(char *handstr,uint8_t cards[7],int32_t verbose);
int32_t cardstr(char *cardstr,uint8_t card);
int32_t init_hexbytes_noT(char *hexbytes,uint8_t *message,long len);
bits256 issue_getpubkey(int32_t *haspubkeyp,char *acct);
uint64_t set_account_NXTSECRET(void *myprivkey,void *mypubkey,char *NXTacct,char *NXTaddr,char *secret,int32_t max,cJSON *argjson,char *coinstr,char *serverport,char *userpass);
int32_t pangea_anotherhand(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t sleepflag);


#define PANGEA_COMMANDS "start", "newtable", "status", "turn"

char *PLUGNAME(_methods)[] = { PANGEA_COMMANDS };
char *PLUGNAME(_pubmethods)[] = { PANGEA_COMMANDS };
char *PLUGNAME(_authmethods)[] = { PANGEA_COMMANDS };

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}

void pangea_sendcmd(char *hex,union hostnet777 *hn,char *cmdstr,int32_t destplayer,uint8_t *data,int32_t datalen,int32_t cardi,int32_t turni)
{
    int32_t i,j,card,n,hexlen,blindflag = 0; uint64_t destbits; bits256 destpub; cJSON *json,*array; char hoststr[1024];
    struct cards777_pubdata *dp = hn->client->H.pubdata;
    hoststr[0] = 0;
    if ( hn->client->H.slot == 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<5; i++)
        {
            if ( (card= dp->hand.community[i]) != 0xff )
                jaddinum(array,card);
            else break;
        }
        if ( i > 0 )
        {
            sprintf(hoststr,"\"state\":%u,\"community\":%s,",hn->client->H.state,jprint(array,1));
            cardi = dp->N*2 + i;
        }
    }
    sprintf(hex,"{\"cmd\":\"%s\",\"millitime\":\"%lld\",\"turni\":%d,\"myind\":%d,\"cardi\":%d,\"dest\":%d,\"sender\":\"%llu\",\"timestamp\":\"%lu\",\"n\":%u,%s\"data\":\"",cmdstr,(long long)hostnet777_convmT(&hn->client->H.mT,0),turni,hn->client->H.slot,cardi,destplayer,(long long)hn->client->H.nxt64bits,time(NULL),datalen,hoststr);
    if ( data != 0 && datalen != 0 )
    {
        n = (int32_t)strlen(hex);
        init_hexbytes_noT(&hex[n],data,datalen);
    }
    strcat(hex,"\"}");
    if ( (json= cJSON_Parse(hex)) == 0 )
    {
        printf("error creating json\n");
        return;
    }
    free_json(json);
    hexlen = (int32_t)strlen(hex)+1;
    //printf("HEX.[%s] hexlen.%d n.%d\n",hex,hexlen,datalen);
    if ( destplayer < 0 || ((1LL << destplayer) & dp->pmworks) == 0 )
    {
        if ( hn->client->H.slot == 0 && destplayer < 0 )
        {
            //queue_enqueue("host777",&hn->server->H.Q,(void *)hex);
            for (j=1; j<dp->N; j++)
                if ( j != hn->client->H.slot )
                {
                    destpub = dp->playerpubs[j];
                    destbits = acct777_nxt64bits(destpub);
                    hostnet777_msg(destbits,destpub,hn,blindflag,hex,hexlen);
                }
        }
        destbits = 0;
        memset(destpub.bytes,0,sizeof(destpub));
    }
    else
    {
        destpub = dp->playerpubs[destplayer];
        destbits = acct777_nxt64bits(destpub);
    }
    hostnet777_msg(destbits,destpub,hn,blindflag,hex,hexlen);
}

#include "pangeafunds.c"

bits256 pangea_destpub(uint64_t destbits)
{
    int32_t i,haspubkey; bits256 destpub; char destNXT[64];
    memset(destpub.bytes,0,sizeof(destpub));
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        if ( TABLES[i] != 0 && TABLES[i]->tp->nxt64bits == destbits )
        {
            destpub = TABLES[i]->tp->hn.client->H.pubkey;
            break;
        }
    if ( i == sizeof(TABLES)/sizeof(*TABLES) )
    {
        expand_nxt64bits(destNXT,destbits);
        destpub = issue_getpubkey(&haspubkey,destNXT);
    }
    return(destpub);
}

struct pangea_info *pangea_find(uint64_t tableid,int32_t threadid)
{
    int32_t i;
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        if ( TABLES[i] != 0 && tableid == TABLES[i]->tableid && (threadid < 0 || TABLES[i]->tp->threadid == threadid) )
            return(TABLES[i]);
    return(0);
}

struct pangea_info *pangea_find64(uint64_t tableid,uint64_t nxt64bits)
{
    int32_t i,j;
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
    {
        if ( TABLES[i] != 0 && tableid == TABLES[i]->tableid && TABLES[i]->tp != 0  )
        {
            for (j=0; j<TABLES[i]->numaddrs; j++)
            {
                if ( TABLES[i]->addrs[j] == nxt64bits )
                    return(TABLES[i]);
            }
        }
    }
    return(0);
}

void pangea_free(struct pangea_info *sp)
{
    int32_t i;
    for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        if ( TABLES[i] == sp )
        {
            TABLES[i] = 0;
            break;
        }
    printf("PANGEA PURGE %llu\n",(long long)sp->tableid);
    free(sp);
}

void pangea_clearhand(struct cards777_handinfo *hand)
{
    bits256 *final,*cardpubs; int32_t i;
    final = hand->final, cardpubs = hand->cardpubs;
    memset(hand,0,sizeof(*hand));
    hand->final = final, hand->cardpubs = cardpubs;
    for (i=0; i<5; i++)
        hand->community[i] = 0xff;
    memset(hand->hands,0xff,sizeof(hand->hands));
}

void pangea_antes(union hostnet777 *hn,struct cards777_pubdata *dp)
{
    int32_t i,j;
    for (i=0; i<dp->N; i++)
        if ( dp->balances[i] <= 0 )
            pangea_fold(dp,i);
    if ( dp->ante != 0 )
    {
        for (i=0; i<dp->N; i++)
        {
            if ( i != dp->button && i != (dp->button+1) % dp->N )
            {
                if ( dp->balances[i] < dp->ante )
                    pangea_fold(dp,i);
                else pangea_bet(hn,dp,i,dp->ante);
            }
        }
    }
    for (i=0; i<dp->N; i++)
    {
        j = (dp->button + i) % dp->N;
        if ( dp->balances[j] < (dp->bigblind >> 1) )
            pangea_fold(dp,j);
        else
        {
            dp->button = j;
            pangea_bet(hn,dp,dp->button,(dp->bigblind>>1));
            break;
        }
    }
    for (i=1; i<dp->N; i++)
    {
        j = (dp->button + i) % dp->N;
        if ( dp->balances[j] < dp->bigblind )
            pangea_fold(dp,j);
        else
        {
            pangea_bet(hn,dp,j,dp->bigblind);
            break;
        }
    }
}

void pangea_sendnewdeck(union hostnet777 *hn,struct cards777_pubdata *dp)
{
    int32_t hexlen; bits256 destpub;
    hexlen = (int32_t)strlen(dp->newhand)+1;
    memset(destpub.bytes,0,sizeof(destpub));
    hostnet777_msg(0,destpub,hn,0,dp->newhand,hexlen);
    dp->startdecktime = (uint32_t)time(NULL);
}

int32_t pangea_newdeck(union hostnet777 *src)
{
    uint8_t data[(CARDS777_MAXCARDS + 1) * sizeof(bits256)]; struct cards777_pubdata *dp; char nrs[512]; struct cards777_privdata *priv; int32_t n,len;
    dp = src->client->H.pubdata;
    priv = src->client->H.privdata;
    pangea_clearhand(&dp->hand);
    priv->hole[0] = priv->hole[1] = 0xff;
    memset(priv->holecards,0,sizeof(priv->holecards));
    init_sharenrs(dp->hand.sharenrs,0,dp->N,dp->N);
    dp->hand.checkprod = dp->hand.cardpubs[dp->numcards] = cards777_initdeck(priv->outcards,dp->hand.cardpubs,dp->numcards,dp->N,dp->playerpubs,0);
    init_hexbytes_noT(nrs,dp->hand.sharenrs,dp->N);
    len = (dp->numcards + 1) * sizeof(bits256);
    sprintf(dp->newhand,"{\"cmd\":\"%s\",\"millitime\":\"%lld\",\"sender\":\"%llu\",\"timestamp\":\"%lu\",\"sharenrs\":\"%s\",\"n\":%u,\"data\":\"","newhand",(long long)hostnet777_convmT(&src->server->H.mT,0),(long long)src->client->H.nxt64bits,time(NULL),nrs,len);
    n = (int32_t)strlen(dp->newhand);
    memcpy(data,dp->hand.cardpubs,len);
    init_hexbytes_noT(&dp->newhand[n],data,len);
    strcat(dp->newhand,"\"}");
    pangea_sendnewdeck(src,dp);
    printf("host sends NEWDECK encode.%llx numhands.%d\n",(long long)priv->outcards[0].txid,dp->numhands);
    return(0);
}

int32_t pangea_newhand(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    char *nrs; char hex[1024];
    if ( data == 0 || datalen != (dp->numcards + 1) * sizeof(bits256) )
    {
        printf("pangea_newhand invalid datalen.%d vs %ld\n",datalen,(dp->numcards + 1) * sizeof(bits256));
        return(-1);
    }
    printf("NEWHAND received\n");
    if ( hn->server->H.slot != 0 )
        pangea_clearhand(&dp->hand);
    priv->hole[0] = priv->hole[1] = 0xff;
    memset(priv->holecards,0,sizeof(priv->holecards));
    dp->numhands++;
    dp->button++;
    if ( dp->button >= dp->N )
        dp->button = 0;
    memcpy(dp->hand.cardpubs,data,(dp->numcards + 1) * sizeof(bits256));
    dp->hand.checkprod = cards777_pubkeys(dp->hand.cardpubs,dp->numcards,dp->hand.cardpubs[dp->numcards]);
    //printf("player.%d (%llx vs %llx) got cardpubs.%llx\n",hn->client->H.slot,(long long)hn->client->H.pubkey.txid,(long long)dp->playerpubs[hn->client->H.slot].txid,(long long)dp->checkprod.txid);
    if ( (nrs= jstr(json,"sharenrs")) != 0 )
        decode_hex(dp->hand.sharenrs,(int32_t)strlen(nrs)>>1,nrs);
    pangea_antes(hn,dp);
    pangea_sendcmd(hex,hn,"gotdeck",-1,dp->hand.checkprod.bytes,sizeof(uint64_t),dp->hand.cardi,dp->hand.userinput_starttime);
    return(0);
}

int32_t pangea_card(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t cardi,int32_t senderind)
{
    int32_t destplayer,card,selector,validcard = -1; bits256 cardpriv; char hex[1024],cardAstr[8],cardBstr[8];
    if ( data == 0 || datalen != sizeof(bits256) )
    {
        printf("pangea_card invalid datalen.%d vs %ld\n",datalen,sizeof(bits256));
        return(-1);
    }
    //printf("pangea_card priv.%llx\n",(long long)hn->client->H.privkey.txid);
    destplayer = juint(json,"dest");
    if ( (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,destplayer,hn->client->H.privkey,dp->hand.cardpubs,dp->numcards,*(bits256 *)data)) >= 0 )
    {
        if ( Debuglevel > 2 )
            printf("player.%d got card.[%d]\n",hn->client->H.slot,card);
        memcpy(&priv->incards[cardi*dp->N + destplayer],cardpriv.bytes,sizeof(bits256));
        selector = (cardi / dp->N);
        priv->holecards[selector] = cardpriv;
        priv->cardis[selector] = cardi;
        dp->hand.hands[destplayer][5 + selector] = priv->hole[selector] = cardpriv.bytes[1];
        validcard = 1;
        cardAstr[0] = cardBstr[0] = 0;
        if ( priv->hole[0] != 0xff )
            cardstr(cardAstr,priv->hole[0]);
        if ( priv->hole[1] != 0xff )
            cardstr(cardBstr,priv->hole[1]);
        printf(">>>>>>>>>> dest.%d priv.%p holecards[%d] cardi.%d / dp->N %d (%d %d) -> (%s %s)\n",destplayer,priv,priv->hole[cardi / dp->N],cardi,dp->N,priv->hole[0],priv->hole[1],cardAstr,cardBstr);
    } else printf("ERROR player.%d got no card %llx\n",hn->client->H.slot,*(long long *)data);
    if ( cardi < dp->N*2 )
        pangea_sendcmd(hex,hn,"facedown",-1,(void *)&cardi,sizeof(cardi),cardi,validcard);
    else pangea_sendcmd(hex,hn,"faceup",-1,cardpriv.bytes,sizeof(cardpriv),cardi,-1);
    return(0);
}

int32_t pangea_preflop(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    char hex[2 * CARDS777_MAXPLAYERS * CARDS777_MAXCARDS * sizeof(bits256)]; int32_t i,card,iter,cardi,destplayer; bits256 cardpriv,decoded;
    if ( data == 0 || datalen != (2 * dp->N) * (dp->N * sizeof(bits256)) )
    {
        printf("pangea_preflop invalid datalen.%d vs %ld\n",datalen,(2 * dp->N) * sizeof(bits256));
        return(-1);
    }
    printf("preflop player.%d\n",hn->client->H.slot);
    memcpy(priv->incards,data,datalen);
    if ( hn->client->H.slot > 1 )
    {
        //for (i=0; i<dp->numcards*dp->N; i++)
        //    printf("%llx ",(long long)priv->outcards[i].txid);
        //printf("player.%d outcards\n",hn->client->H.slot);
        for (cardi=0; cardi<dp->N*2; cardi++)
            for (destplayer=0; destplayer<dp->N; destplayer++)
            {
                if ( 0 && (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,destplayer,hn->client->H.privkey,dp->hand.cardpubs,dp->numcards,priv->incards[cardi*dp->N + destplayer])) >= 0 )
                    printf("ERROR: unexpected decode player.%d got card.[%d]\n",hn->client->H.slot,card);
                decoded = cards777_decode(priv->xoverz,destplayer,priv->incards[cardi*dp->N + destplayer],priv->outcards,dp->numcards,dp->N);
                *(bits256 *)&data[(cardi*dp->N + destplayer) * sizeof(bits256)] = decoded;
            }
        printf("issue preflop\n");
        pangea_sendcmd(hex,hn,"preflop",hn->client->H.slot-1,data,datalen,dp->N * 2 * dp->N,-1);
    }
    else
    {
        printf("sendout cards\n");
        for (iter=cardi=0; iter<2; iter++)
            for (i=0; i<dp->N; i++,cardi++)
            {
                destplayer = (dp->button + i) % dp->N;
                decoded = cards777_decode(priv->xoverz,destplayer,priv->incards[cardi*dp->N + destplayer],priv->outcards,dp->numcards,dp->N);
                //printf("[%llx -> %llx] ",*(long long *)&data[(cardi*dp->N + destplayer) * sizeof(bits256)],(long long)decoded.txid);
                if ( destplayer == hn->client->H.slot )
                    pangea_card(hn,json,dp,priv,decoded.bytes,sizeof(bits256),cardi,destplayer);
                else pangea_sendcmd(hex,hn,"card",destplayer,decoded.bytes,sizeof(bits256),cardi,-1);
            }
    }
    return(0);
}

int32_t pangea_encoded(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    char *hex;
    if ( data == 0 || datalen != (dp->numcards * dp->N) * sizeof(bits256) )
    {
        printf("pangea_encode invalid datalen.%d vs %ld (%s)\n",datalen,(dp->numcards * dp->N) * sizeof(bits256),jprint(json,0));
        return(-1);
    }
    cards777_encode(priv->outcards,priv->xoverz,priv->allshares,priv->myshares,dp->hand.sharenrs,dp->M,(void *)data,dp->numcards,dp->N);
    //int32_t i; for (i=0; i<dp->numcards*dp->N; i++)
    //    printf("%llx ",(long long)priv->outcards[i].txid);
    printf("player.%d encodes into %p %llx -> %llx\n",hn->client->H.slot,priv->outcards,*(uint64_t *)data,(long long)priv->outcards[0].txid);
    if ( hn->client->H.slot != 0 && (hex= malloc(65536)) != 0 )
    {
        if ( hn->client->H.slot < dp->N-1 )
        {
            printf("send encoded\n");
            pangea_sendcmd(hex,hn,"encoded",hn->client->H.slot+1,priv->outcards[0].bytes,datalen,dp->N*dp->numcards,-1);
        }
        else
        {
            pangea_sendcmd(hex,hn,"final",-1,priv->outcards[0].bytes,datalen,dp->N*dp->numcards,-1);
            pangea_preflop(hn,json,dp,priv,priv->outcards[0].bytes,(2 * dp->N) * (dp->N * sizeof(bits256)),hn->client->H.slot);
        }
        free(hex);
    }
    return(0);
}

int32_t pangea_final(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    if ( data == 0 || datalen != (dp->numcards * dp->N) * sizeof(bits256) )
    {
        printf("pangea_final invalid datalen.%d vs %ld\n",datalen,(dp->numcards * dp->N) * sizeof(bits256));
        return(-1);
    }
    //if ( Debuglevel > 2 )
        printf("player.%d final into %p\n",hn->client->H.slot,priv->outcards);
    memcpy(dp->hand.final,data,sizeof(bits256) * dp->N * dp->numcards);
    if ( hn->client->H.slot == dp->N-1 )
        memcpy(priv->incards,data,sizeof(bits256) * dp->N * dp->numcards);
    return(0);
}

int32_t pangea_facedown(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t cardi,int32_t senderind)
{
    int32_t i,validcard,n = 0;
    if ( data == 0 || datalen != sizeof(int32_t) )
    {
        printf("pangea_facedown invalid datalen.%d vs %ld\n",datalen,sizeof(bits256));
        return(-1);
    }
    validcard = juint(json,"turni");
    if ( validcard > 0 )
        dp->hand.havemasks[senderind] |= (1LL << cardi);
    for (i=0; i<dp->N; i++)
    {
        if ( Debuglevel > 2 )
            printf("%llx ",(long long)dp->hand.havemasks[i]);
        if ( bitweight(dp->hand.havemasks[i]) == 2 )
            n++;
    }
    //if ( Debuglevel > 2 )
    printf(" | player.%d sees that destplayer.%d got cardi.%d valid.%d | %llx | n.%d\n",hn->client->H.slot,senderind,cardi,validcard,(long long)dp->hand.havemasks[senderind],n);
    if ( hn->client->H.slot == 0 && n == dp->N )
        pangea_startbets(hn,dp,dp->N*2);
    return(0);
}

int32_t pangea_decode(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    int32_t cardi,destplayer,card,turni; bits256 cardpriv,decoded; char hex[1024];
    if ( data == 0 || datalen != sizeof(bits256) )
    {
        printf("pangea_decode invalid datalen.%d vs %ld\n",datalen,sizeof(bits256));
        return(-1);
    }
    cardi = juint(json,"cardi");
    turni = juint(json,"turni");
    if ( cardi < dp->N*2 || cardi >= dp->N*2 + 5 )
    {
        printf("pangea_decode invalid cardi.%d\n",cardi);
        return(-1);
    }
    destplayer = 0;
    memcpy(&priv->incards[cardi*dp->N + destplayer],data,sizeof(bits256));
    if ( turni == hn->client->H.slot )
    {
        if ( hn->client->H.slot > 0 )
        {
            decoded = cards777_decode(priv->xoverz,destplayer,priv->incards[cardi*dp->N + destplayer],priv->outcards,dp->numcards,dp->N);
            pangea_sendcmd(hex,hn,"decoded",-1,decoded.bytes,sizeof(decoded),cardi,hn->client->H.slot-1);
            //printf("player.%d decoded cardi.%d %llx -> %llx\n",hn->client->H.slot,cardi,(long long)priv->incards[cardi*dp->N + destplayer].txid,(long long)decoded.txid);
        }
        else
        {
            if ( (card= cards777_checkcard(&cardpriv,cardi,hn->client->H.slot,hn->client->H.slot,hn->client->H.privkey,dp->hand.cardpubs,dp->numcards,*(bits256 *)data)) >= 0 )
            {
                printf("player.%d decoded cardi.%d card.[%d]\n",hn->client->H.slot,cardi,card);
                pangea_sendcmd(hex,hn,"faceup",-1,cardpriv.bytes,sizeof(cardpriv),cardi,cardpriv.txid!=0?1:-1);
            }
        }
    }
    return(0);
}

uint32_t pangea_rank(struct cards777_pubdata *dp,int32_t senderind)
{
    int32_t i; char handstr[128];
    if ( dp->hand.handranks[senderind] != 0 )
        return(dp->hand.handranks[senderind]);
    for (i=0; i<7; i++)
    {
        if ( i < 5 )
            dp->hand.hands[senderind][i] = dp->hand.community[i];
        if ( dp->hand.hands[senderind][i] == 0xff )
            break;
    }
    if ( i == 7 )
    {
        dp->hand.handranks[senderind] = set_handstr(handstr,dp->hand.hands[senderind],0);
        dp->hand.handmask |= (1 << senderind);
        printf("sender.%d (%s) rank.%x handmask.%x\n",senderind,handstr,dp->hand.handranks[senderind],dp->hand.handmask);
    }
    return(dp->hand.handranks[senderind]);
}

int32_t pangea_faceup(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    int32_t cardi,validcard,i; char hexstr[65];
    if ( data == 0 || datalen != sizeof(bits256) )
    {
        printf("pangea_faceup invalid datalen.%d vs %ld\n",datalen,(dp->numcards + 1) * sizeof(bits256));
        return(-1);
    }
    init_hexbytes_noT(hexstr,data,sizeof(bits256));
    cardi = juint(json,"cardi");
    validcard = juint(json,"turni");
    //if ( Debuglevel > 2 || hn->client->H.slot == 0 )
        printf("from.%d -> player.%d COMMUNITY.[%d] (%s) cardi.%d valid.%d\n",senderind,hn->client->H.slot,data[1],hexstr,cardi,validcard);
    if ( validcard > 0 && cardi >= dp->N*2 && cardi < dp->N*2+5 )
    {
        dp->hand.community[cardi - dp->N*2] = data[1];
        for (i=0; i<dp->N; i++)
            dp->hand.hands[i][cardi - dp->N*2] = data[1];
        memcpy(dp->hand.community256[cardi - dp->N*2].bytes,data,sizeof(bits256));
        //printf("set community[%d] <- %d\n",cardi - dp->N*2,data[1]);
        if ( senderind == hn->client->H.slot )
            pangea_rank(dp,senderind);
        //if ( hn->client->H.slot == 0 && cardi >= dp->N*2+2 && cardi < dp->N*2+5 )
        //    pangea_startbets(hn,dp,cardi+1);
    }
    else
    {
        printf("valid.%d cardi.%d vs N.%d\n",validcard,cardi,dp->N);
        if ( cardi < dp->N*2 )
        {
            memcpy(dp->hand.cards[senderind][cardi/dp->N].bytes,data,sizeof(bits256));
            dp->hand.hands[senderind][5 + cardi/dp->N] = data[1];
            pangea_rank(dp,senderind);
        }
    }
    return(0);
}

void pangea_serverstate(union hostnet777 *hn,struct cards777_pubdata *dp,struct cards777_privdata *priv)
{
    int32_t i,j,n; struct pangea_info *sp = dp->table;
    if ( dp->hand.finished != 0 && time(NULL) > dp->hand.finished+5 )
        pangea_anotherhand(hn,dp,0);
    if ( time(NULL) > sp->timestamp+20 && time(NULL) < sp->timestamp+22 && dp->pmworks != ((1 << dp->N) - 1) )
    {
        //printf("PMs are only partially working: %llx vs %x, activate selective PUB\n",(long long)dp->pmworks,((1 << dp->N) - 1));
    }
    if ( dp->hand.betstarted == 0 && dp->newhand[0] == 0 )
    {
        static uint32_t disptime;
        for (i=n=0; i<dp->N; i++)
        {
            if ( Debuglevel > 2 )
                printf("%llx ",(long long)dp->hand.havemasks[i]);
            if ( bitweight(dp->hand.havemasks[i]) == 2 )
                n++;
        }
        if ( n < dp->N )
        {
            for (i=0; i<dp->N; i++)
            {
                if ( dp->balances[i] < dp->minbuyin*dp->bigblind || dp->balances[i] > dp->maxbuyin*dp->bigblind )
                    break;
            }
            if ( i == dp->N )
            {
                if ( time(NULL) > dp->startdecktime+10 )
                {
                    printf("send newdeck len.%ld\n",strlen(dp->newhand));
                    pangea_newdeck(hn);
                    printf("sent newdeck %ld\n",strlen(dp->newhand));
                }
            } else if ( disptime != time(NULL) && (time(NULL) % 60) == 0 )
            {
                disptime = (uint32_t)time(NULL);
                for (j=0; j<dp->N; j++)
                    printf("%.8f ",dstr(dp->balances[j]));
                printf("no buyin for %d (%.8f %.8f)\n",i,dstr(dp->minbuyin*dp->bigblind),dstr(dp->maxbuyin*dp->bigblind));
            }
        }
    }
    else if ( dp->newhand[0] != 0 )
    {
        for (i=n=0; i<dp->N; i++)
        {
            if ( Debuglevel > 2 )
                printf("%llx ",(long long)dp->hand.havemasks[i]);
            if ( bitweight(dp->hand.havemasks[i]) == 2 )
                n++;
        }
        if ( n != dp->N )
        {
            for (i=0; i<dp->N; i++)
            {
                if ( dp->othercardpubs[i] != dp->hand.checkprod.txid )
                    break;
            }
            if ( i == dp->N )
            {
                printf("SERVERSTATE issues encoded\n");
                pangea_sendcmd(dp->newhand,hn,"encoded",1,priv->outcards[0].bytes,sizeof(bits256)*dp->N*dp->numcards,dp->N*dp->numcards,-1);
                dp->newhand[0] = 0;
            }
            else if ( 0 && dp->startdecktime != 0 && time(NULL) > dp->startdecktime+10 )
            {
                pangea_sendnewdeck(hn,dp);
                printf("resend NEWDECK encode.%llx numhands.%d\n",(long long)priv->outcards[0].txid,dp->numhands);
            }
        }
    }
    else if ( 0 && dp->hand.final[0].txid == 0 )
    {
        if ( dp->startdecktime != 0 && time(NULL) > dp->startdecktime+20 )
        {
            printf("startdecktim.%u host didnt get final in time, restarting hand\n",dp->startdecktime);
            pangea_sendnewdeck(hn,dp);
        }
    }
}

int32_t pangea_gotdeck(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    dp->othercardpubs[senderind] = *(uint64_t *)data;
    printf("player.%d got pangea_gotdeck from senderind.%d otherpubs.%llx\n",hn->client->H.slot,senderind,(long long)dp->othercardpubs[senderind]);
    if ( hn->client->H.slot == 0 )
        pangea_serverstate(hn,dp,priv);
    return(0);
}

int32_t pangea_ready(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    char hex[2048];
    dp->readymask |= (1 << senderind);
    printf("player.%d got ready from senderind.%d readymask.%x\n",hn->client->H.slot,senderind,dp->readymask);
    if ( 0 && hn->client->H.slot == 0 )
    {
        if ( (dp->pmworks & (1 << senderind)) == 0 )
        {
            printf("send pmtest from %d to %d\n",hn->client->H.slot,senderind);
            pangea_sendcmd(hex,hn,"pmtest",-1,dp->hand.checkprod.bytes,sizeof(uint64_t),-1,-1);
            pangea_sendcmd(hex,hn,"pmtest",senderind,dp->hand.checkprod.bytes,sizeof(uint64_t),-1,senderind);
        }
    }
    return(0);
}

int32_t pangea_pmtest(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    int32_t turni,cardi; char hex[2048];
    cardi = juint(json,"cardi");
    turni = juint(json,"turni");
    if ( senderind >= 0 && senderind < dp->N )
    {
        if ( cardi < 0 )
        {
            if ( turni >= 0 )
                dp->pmviaworks |= (1 << senderind);
            else dp->broadcastworks |= (1 << senderind);
        }
        else
        {
            if ( turni >= 0 )
                dp->pmworks |= (1 << senderind);
            else dp->pmviaworks |= (1 << senderind);
        }
        if ( dp->pmworks != ((1 << dp->N) - 1) )
        {
            //printf("PMworks: %x %x %x\n",dp->pmworks,dp->pmviaworks,dp->broadcastworks);
        }
    }
    //printf("got pmtest.%d from %d cardi.%d\n",turni,senderind,cardi);
    if ( hn->client->H.slot == 0 )
    {
        if ( dp->pmworks == ((1 << dp->N) - 1) )
        {
            //printf("all pms work\n");
        }
    }
    else
    {
        if ( dp->pmworks != ((1 << dp->N) - 1) )
        {
            pangea_sendcmd(hex,hn,"pmtest",senderind,dp->hand.checkprod.bytes,sizeof(uint64_t),senderind,turni);
            pangea_sendcmd(hex,hn,"pmtest",-1,dp->hand.checkprod.bytes,sizeof(uint64_t),-1,turni);
        }
    }
    return(0);
}

int32_t pangea_ping(union hostnet777 *hn,cJSON *json,struct cards777_pubdata *dp,struct cards777_privdata *priv,uint8_t *data,int32_t datalen,int32_t senderind)
{
    dp->othercardpubs[senderind] = *(uint64_t *)data;
    if ( senderind == 0 )
    {
        /*dp->hand.undergun = juint(json,"turni");
        dp->hand.cardi = juint(json,"cardi");
        if ( (array= jarray(&n,json,"community")) != 0 )
        {
            for (i=0; i<n; i++)
                dp->hand.community[i] = juint(jitem(array,i),0);
        }*/
    }
    //printf("player.%d GOTPING.(%s) %llx\n",hn->client->H.slot,jprint(json,0),(long long)dp->othercardpubs[senderind]);
    return(0);
}

int32_t pangea_anotherhand(union hostnet777 *hn,struct cards777_pubdata *dp,int32_t sleepflag)
{
    int32_t i,n,activej = -1; uint64_t total = 0;
    for (i=n=0; i<dp->N; i++)
    {
        total += dp->balances[i];
        printf("(p%d %.8f) ",i,dstr(dp->balances[i]));
        if ( dp->balances[i] != 0 )
        {
            if ( activej < 0 )
                activej = i;
            n++;
        }
    }
    printf("balances %.8f [%.8f]\n",dstr(total),dstr(total + dp->hostrake + dp->pangearake));
    if ( n == 1 )
    {
        printf("Only player.%d left with %.8f | get sigs and cashout after numhands.%d\n",activej,dstr(dp->balances[activej]),dp->numhands);
        return(1);
    }
    else
    {
        if ( sleepflag != 0 )
            sleep(sleepflag);
        pangea_newdeck(hn);
        if ( sleepflag != 0 )
            sleep(sleepflag);
    }
    return(n);
}

void pangea_chat(uint64_t senderbits,void *buf,int32_t len,int32_t senderind)
{
    printf(">>>>>>>>>>> CHAT FROM.%d %llu: (%s)\n",senderind,(long long)senderbits,buf);
}

int32_t pangea_poll(uint64_t *senderbitsp,uint32_t *timestampp,union hostnet777 *hn)
{
    char *jsonstr,*hexstr,*cmdstr; cJSON *json; struct cards777_privdata *priv; struct cards777_pubdata *dp; int32_t len,senderind;
    uint8_t buf[sizeof(bits256) * CARDS777_MAXPLAYERS*CARDS777_MAXCARDS];
    *senderbitsp = 0;
    if ( hn == 0 || hn->client == 0 )
    {
        printf("null hn.%p %p\n",hn,hn!=0?hn->client:0);
        return(-1);
    }
    dp = hn->client->H.pubdata;
    priv = hn->client->H.privdata;
    if ( (jsonstr= queue_dequeue(&hn->client->H.Q,1)) != 0 )
    {
        //printf("player.%d GOT.(%s)\n",hn->client->H.slot,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            *senderbitsp = j64bits(json,"sender");
            if ( (senderind= juint(json,"myind")) < 0 || senderind >= dp->N )
            {
                printf("pangea_poll: illegal senderind.%d cardi.%d turni.%d\n",senderind,juint(json,"cardi"),juint(json,"turni"));
                goto cleanup;
            }
            *timestampp = juint(json,"timestamp");
            hn->client->H.state = juint(json,"state");
            len = juint(json,"n");
            if ( (hexstr= jstr(json,"data")) != 0 && strlen(hexstr) == (len<<1) )
            {
                if ( len > sizeof(buf) )
                {
                    printf("len too big for pangea_poll\n");
                    goto cleanup;
                }
                decode_hex(buf,len,hexstr);
            } else printf("len.%d vs hexlen.%ld (%s)\n",len,strlen(hexstr)>>1,hexstr);
            if ( (cmdstr= jstr(json,"cmd")) != 0 )
            {
                if ( strcmp(cmdstr,"newhand") == 0 )
                    pangea_newhand(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"ping") == 0 )
                    pangea_ping(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"pmtest") == 0 )
                    pangea_pmtest(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"gotdeck") == 0 )
                    pangea_gotdeck(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"ready") == 0 )
                    pangea_ready(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"encoded") == 0 )
                    pangea_encoded(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"final") == 0 )
                    pangea_final(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"addfunds") == 0 )
                    pangea_addfunds(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"preflop") == 0 )
                    pangea_preflop(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"decoded") == 0 )
                    pangea_decode(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"card") == 0 )
                    pangea_card(hn,json,dp,priv,buf,len,juint(json,"cardi"),senderind);
                else if ( strcmp(cmdstr,"facedown") == 0 )
                    pangea_facedown(hn,json,dp,priv,buf,len,juint(json,"cardi"),senderind);
                else if ( strcmp(cmdstr,"faceup") == 0 )
                    pangea_faceup(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"turn") == 0 )
                    pangea_turn(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"confirmturn") == 0 )
                    pangea_confirmturn(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"chat") == 0 )
                    pangea_chat(*senderbitsp,buf,len,senderind);
                else if ( strcmp(cmdstr,"action") == 0 )
                    pangea_action(hn,json,dp,priv,buf,len,senderind);
                else if ( strcmp(cmdstr,"showdown") == 0 )
                    pangea_showdown(hn,json,dp,priv,buf,len,senderind);
            }
cleanup:
            free_json(json);
        }
        free_queueitem(jsonstr);
    }
    return(hn->client->H.state);
}

char *pangea_status(uint64_t my64bits,uint64_t tableid,cJSON *json)
{
    int32_t i,j,threadid = 0; struct pangea_info *sp; cJSON *item,*array=0,*retjson = 0;
    if ( tableid != 0 )
    {
        if ( (sp= pangea_find(tableid,threadid)) != 0 )
        {
            if ( (item= pangea_tablestatus(sp)) != 0 )
            {
                retjson = cJSON_CreateObject();
                jaddstr(retjson,"result","success");
                jadd(retjson,"table",item);
                return(jprint(retjson,1));
            }
        }
    }
    else
    {
        for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        {
            if ( (sp= TABLES[i]) != 0 )
            {
                for (j=0; j<sp->numaddrs; j++)
                    if ( sp->addrs[j] == my64bits )
                    {
                        if ( (item= pangea_tablestatus(sp)) != 0 )
                        {
                            if ( array == 0 )
                                array = cJSON_CreateArray();
                            jaddi(array,item);
                        }
                        break;
                    }
            }
        }
    }
    retjson = cJSON_CreateObject();
    if ( array == 0 )
        jaddstr(retjson,"error","no table status");
    else
    {
        jaddstr(retjson,"result","success");
        jadd(retjson,"tables",array);
    }
    jadd64bits(retjson,"nxtaddr",my64bits);
    return(jprint(retjson,1));
}

int32_t pangea_idle(struct plugin_info *plugin)
{
    int32_t i,n,m,pinggap = 1; uint64_t senderbits; uint32_t timestamp; struct pangea_thread *tp;
    union hostnet777 *hn; struct cards777_pubdata *dp; char hex[1024];
    while ( 1 )
    {
        for (i=n=m=0; i<_PANGEA_MAXTHREADS; i++)
        {
            if ( (tp= THREADS[i]) != 0 )
            {
                hn = &tp->hn;
                //printf("pangea idle player.%d\n",hn->client->H.slot);
                if ( hn->client->H.done == 0 )
                {
                    n++;
                    if ( hostnet777_idle(hn) != 0 )
                        m++;
                    pangea_poll(&senderbits,&timestamp,hn);
                    if ( hn->client->H.slot == 0 )
                        pinggap = 1;
                    if ( hn->client != 0 )
                    {
                        if ( time(NULL) > hn->client->H.lastping + pinggap )
                        {
                            if ( 1 && (dp= hn->client->H.pubdata) != 0 )
                            {
                                pangea_sendcmd(hex,hn,"ping",-1,dp->hand.checkprod.bytes,sizeof(uint64_t),dp->hand.cardi,dp->hand.undergun);
                                hn->client->H.lastping = (uint32_t)time(NULL);
                            }
                        }
                        if ( hn->client->H.slot == 0 && (dp= hn->client->H.pubdata) != 0 )
                            pangea_serverstate(hn,dp,hn->server->H.privdata);
                    }
                }
            }
        }
        if ( n == 0 )
            break;
        if ( m == 0 )
            msleep(3);
    }
    //for (i=0; i<_PANGEA_MAXTHREADS; i++)
    //    if ( THREADS[i] != 0 && Pangea_waiting != 0 )
    //        pangea_userpoll(&THREADS[i]->hn);
    return(0);
}

void pangea_buyins(uint32_t *minbuyinp,uint32_t *maxbuyinp)
{
    if ( *minbuyinp == 0 && *maxbuyinp == 0 )
    {
        *minbuyinp = 100;
        *maxbuyinp = 250;
    }
    else
    {
        printf("minbuyin.%d maxbuyin.%d -> ",*minbuyinp,*maxbuyinp);
        if ( *minbuyinp < 20 )
            *minbuyinp = 20;
        if ( *maxbuyinp < *minbuyinp )
            *maxbuyinp = (*minbuyinp * 4);
        if ( *maxbuyinp > 1000 )
            *maxbuyinp = 1000;
        if ( *minbuyinp > *maxbuyinp )
            *minbuyinp = *maxbuyinp;
        printf("(%d %d)\n",*minbuyinp,*maxbuyinp);
    }
}

struct pangea_info *pangea_create(struct pangea_thread *tp,int32_t *createdflagp,char *base,uint32_t timestamp,uint64_t *addrs,int32_t numaddrs,uint64_t bigblind,uint64_t ante,uint64_t *isbot,uint32_t minbuyin,uint32_t maxbuyin)
{
    struct pangea_info *sp = 0; bits256 hash; int32_t i,j,numcards,firstslot = -1; struct cards777_privdata *priv; struct cards777_pubdata *dp;
    if ( createdflagp != 0 )
        *createdflagp = 0;
    for (i=0; i<numaddrs; i++)
        printf("%llu ",(long long)addrs[i]);
    printf("pangea_create numaddrs.%d\n",numaddrs);
    for (i=0; i<numaddrs; i++)
    {
        if ( addrs[i] == tp->nxt64bits )
            break;
    }
    if ( i == numaddrs )
    {
        printf("this node not in addrs\n");
        return(0);
    }
    if ( numaddrs > 0 && (sp= calloc(1,sizeof(*sp))) != 0 )
    {
        sp->tp = tp;
        numcards = CARDS777_MAXCARDS;
        pangea_buyins(&minbuyin,&maxbuyin);
        tp->numcards = numcards, tp->N = numaddrs;
        sp->dp = dp = cards777_allocpub((numaddrs >> 1) + 1,numcards,numaddrs);
        dp->minbuyin = minbuyin, dp->maxbuyin = maxbuyin;
        if ( dp == 0 )
        {
            printf("pangea_create: unexpected out of memory pub\n");
            return(0);
        }
        for (j=0; j<5; j++)
            dp->hand.community[j] = 0xff;
        for (j=0; j<numaddrs; j++)
        {
            //if ( balances != 0 )
            //    dp->balances[j] = balances[j];
            //else dp->balances[j] = 100;
            if ( isbot != 0 )
                dp->isbot[j] = isbot[j];
        }
        sp->priv = priv = cards777_allocpriv(numcards,numaddrs);
        priv->hole[0] = priv->hole[1] = 0xff;
        if ( priv == 0 )
        {
            printf("pangea_create: unexpected out of memory priv\n");
            return(0);
        }
        priv->autoshow = Showmode;
        priv->autofold = Autofold;
        strcpy(sp->base,base);
        if ( (sp->timestamp= timestamp) == 0 )
            sp->timestamp = (uint32_t)time(NULL);
        sp->numaddrs = numaddrs;
        sp->basebits = stringbits(base);
        sp->bigblind = dp->bigblind = bigblind, sp->ante = dp->ante = ante;
        memcpy(sp->addrs,addrs,numaddrs * sizeof(sp->addrs[0]));
        vcalc_sha256(0,hash.bytes,(uint8_t *)sp,numaddrs * sizeof(sp->addrs[0]) + 4*sizeof(uint32_t) + 3*sizeof(uint64_t));
        sp->tableid = hash.txid;
        for (i=0; i<sizeof(TABLES)/sizeof(*TABLES); i++)
        {
            if ( TABLES[i] != 0 )
            {
                if ( sp->tableid == TABLES[i]->tableid && tp->threadid == TABLES[i]->tp->threadid )
                {
                    printf("tableid %llu already exists!\n",(long long)sp->tableid);
                    free(sp);
                    return(TABLES[i]);
                }
            }
            else if ( firstslot < 0 )
                firstslot = i;
        }
        TABLES[firstslot] = sp;
        if ( createdflagp != 0 )
            *createdflagp = 1;
    }
    return(sp);
}

cJSON *pangea_ciphersjson(struct cards777_pubdata *dp,struct cards777_privdata *priv)
{
    int32_t i,j,nonz = 0; char hexstr[65]; cJSON *array = cJSON_CreateArray();
    for (i=0; i<dp->numcards; i++)
        for (j=0; j<dp->N; j++,nonz++)
        {
            init_hexbytes_noT(hexstr,priv->outcards[nonz].bytes,sizeof(bits256));
            jaddistr(array,hexstr);
        }
    return(array);
}

cJSON *pangea_playerpubs(bits256 *playerpubs,int32_t num)
{
    int32_t i; char hexstr[65]; cJSON *array = cJSON_CreateArray();
    for (i=0; i<num; i++)
    {
        init_hexbytes_noT(hexstr,playerpubs[i].bytes,sizeof(bits256));
        //printf("(%llx-> %s) ",(long long)playerpubs[i].txid,hexstr);
        jaddistr(array,hexstr);
    }
    //printf("playerpubs\n");
    return(array);
}

cJSON *pangea_cardpubs(struct cards777_pubdata *dp)
{
    int32_t i; char hexstr[65]; cJSON *array = cJSON_CreateArray();
    for (i=0; i<dp->numcards; i++)
    {
        init_hexbytes_noT(hexstr,dp->hand.cardpubs[i].bytes,sizeof(bits256));
        jaddistr(array,hexstr);
    }
    init_hexbytes_noT(hexstr,dp->hand.checkprod.bytes,sizeof(bits256));
    jaddistr(array,hexstr);
    return(array);
}

cJSON *pangea_sharenrs(uint8_t *sharenrs,int32_t n)
{
    int32_t i; cJSON *array = cJSON_CreateArray();
    for (i=0; i<n; i++)
        jaddinum(array,sharenrs[i]);
    return(array);
}

char *pangea_newtable(int32_t threadid,cJSON *json,uint64_t my64bits,bits256 privkey,bits256 pubkey,char *transport,char *ipaddr,uint16_t port,uint32_t minbuyin,uint32_t maxbuyin)
{
    int32_t createdflag,num,i,myind= -1; uint64_t tableid,addrs[CARDS777_MAXPLAYERS],isbot[CARDS777_MAXPLAYERS];
    struct pangea_info *sp; cJSON *array; struct pangea_thread *tp; char *base,*hexstr,*endpoint,hex[1024]; uint32_t timestamp;
    struct cards777_pubdata *dp; struct hostnet777_server *srv;
    printf("T%d NEWTABLE.(%s)\n",threadid,jprint(json,0));
    if ( (tableid= j64bits(json,"tableid")) != 0 && (base= jstr(json,"base")) != 0 && (timestamp= juint(json,"timestamp")) != 0 )
    {
        if ( (array= jarray(&num,json,"addrs")) == 0 || num < 2 || num > CARDS777_MAXPLAYERS )
        {
            printf("no address or illegal num.%d\n",num);
            return(clonestr("{\"error\":\"no addrs or illegal numplayers\"}"));
        }
        for (i=0; i<num; i++)
        {
            addrs[i] = j64bits(jitem(array,i),0);
            if ( addrs[i] == my64bits )
            {
                threadid = myind = i;
                if ( (tp= THREADS[threadid]) == 0 )
                {
                    THREADS[threadid] = tp = calloc(1,sizeof(*THREADS[threadid]));
                    if ( i == 0 )
                    {
                        if ( (srv= hostnet777_server(privkey,pubkey,transport,ipaddr,port,num)) == 0 )
                            printf("cant create hostnet777 server\n");
                        else
                        {
                            tp->hn.server = srv;
                            memcpy(srv->H.privkey.bytes,privkey.bytes,sizeof(bits256));
                            memcpy(srv->H.pubkey.bytes,pubkey.bytes,sizeof(bits256));
                        }
                    }
                    else
                    {
                        PANGEA_MAXTHREADS = 1;
                        if ( (endpoint= jstr(json,"pangea_endpoint")) != 0 )
                        {
                            if ( strncmp(endpoint,"tcp://127.0.0.1",strlen("tcp://127.0.0.1")) == 0 || strncmp(endpoint,"ws://127.0.0.1",strlen("ws://127.0.0.1")) == 0 )
                            {
                                printf("ILLEGAL pangea_endpoint.(%s)\n",endpoint);
                                return(clonestr("{\"error\":\"contact pangea host and tell them to add myipaddr to their SuperNET.conf\"}"));
                            }
                            if ( (tp->hn.client= hostnet777_client(privkey,pubkey,endpoint,i)) == 0 )
                            {
                                memcpy(tp->hn.client->H.privkey.bytes,privkey.bytes,sizeof(bits256));
                                memcpy(tp->hn.client->H.pubkey.bytes,pubkey.bytes,sizeof(bits256));
                            }
                        }
                    }
                    tp->nxt64bits = my64bits;
                }
            }
        }
        if ( myind < 0 )
            return(clonestr("{\"error\":\"this table is not for me\"}"));
        /*if ( (array= jarray(&num,json,"balances")) == 0 )
        {
            printf("no balances or illegal num.%d\n",num);
            return(clonestr("{\"error\":\"no balances or illegal numplayers\"}"));
        }
        for (i=0; i<num; i++)
            balances[i] = j64bits(jitem(array,i),0);*/
        if ( (array= jarray(&num,json,"isbot")) != 0 )
        {
            for (i=0; i<num; i++)
                isbot[i] = j64bits(jitem(array,i),0);
        }
        else memset(isbot,0,sizeof(isbot));
        printf("call pangea_create\n");
        if ( (sp= pangea_create(tp,&createdflag,base,timestamp,addrs,num,j64bits(json,"bigblind"),j64bits(json,"ante"),isbot,minbuyin,maxbuyin)) == 0 )
        {
            printf("cant create table.(%s) numaddrs.%d\n",base,num);
            return(clonestr("{\"error\":\"cant create table\"}"));
        }
        printf("back from pangea_create\n");
        dp = sp->dp; sp->myind = myind;
        dp->rakemillis = juint(json,"rakemillis");
        if ( dp->rakemillis > PANGEA_MAX_HOSTRAKE )
            dp->rakemillis = PANGEA_MAX_HOSTRAKE;
        dp->table = sp;
        tp->numcards = dp->numcards, tp->N = dp->N, tp->M = dp->M;
        if ( threadid == 0 )
        {
            tp->hn.server->clients[0].pubdata = dp;
            tp->hn.server->clients[0].privdata = sp->priv;
            tp->hn.server->H.pubdata = dp;
            tp->hn.server->H.privdata = sp->priv;
        }
        else
        {
            tp->hn.client->my.pubdata = dp;
            tp->hn.client->my.privdata = sp->priv;
            tp->hn.client->H.pubdata = dp;
            tp->hn.client->H.privdata = sp->priv;
            if ( THREADS[0] != 0 )
            {
                THREADS[0]->hn.server->clients[threadid].pubdata = dp;
                THREADS[0]->hn.server->clients[threadid].privdata = sp->priv;
            }
        }
        if ( (array= jarray(&num,json,"playerpubs")) == 0 || num < 2 || num > CARDS777_MAXPLAYERS )
        {
            printf("no address or illegal num.%d\n",num);
            return(clonestr("{\"error\":\"no addrs or illegal numplayers\"}"));
        }
        for (i=0; i<num; i++)
        {
            hexstr = jstr(jitem(array,i),0);
            decode_hex(dp->playerpubs[i].bytes,sizeof(bits256),hexstr);
            printf("set playerpubs.(%s) %llx\n",hexstr,(long long)dp->playerpubs[i].txid);
            if ( dp->playerpubs[i].txid == 0 )
            {
                printf("player.%d has no NXT pubkey\n",i);
                return(clonestr("{\"error\":\"not all players have published NXT pubkeys\"}"));
            }
        }
        if ( myind >= 0 && createdflag != 0 && addrs[myind] == tp->nxt64bits )
        {
            memcpy(sp->addrs,addrs,sizeof(*addrs) * dp->N);
            dp->readymask |= (1 << sp->myind);
            pangea_sendcmd(hex,&tp->hn,"ready",-1,0,0,0,0);
            return(clonestr("{\"result\":\"newtable created\"}"));
        }
        else if ( createdflag == 0 )
        {
            if ( sp->addrs[0] == tp->nxt64bits )
                return(clonestr("{\"result\":\"this is my table\"}"));
            else return(clonestr("{\"result\":\"table already exists\"}"));
        }
    }
    return(clonestr("{\"error\":\"no tableid\"}"));
}

struct pangea_thread *pangea_threadinit(struct plugin_info *plugin,int32_t maxplayers)
{
    struct pangea_thread *tp; struct hostnet777_server *srv;
    PANGEA_MAXTHREADS = 1;
    THREADS[0] = tp = calloc(1,sizeof(*THREADS[0]));
    if ( tp == 0 )
    {
        printf("pangea_threadinit: unexpected out of memory\n");
        return(0);
    }
    tp->nxt64bits = plugin->nxt64bits;
    if ( (srv= hostnet777_server(*(bits256 *)plugin->mypriv,*(bits256 *)plugin->mypub,plugin->transport,plugin->ipaddr,plugin->pangeaport,9)) == 0 )
        printf("cant create hostnet777 server\n");
    else
    {
        tp->hn.server = srv;
        memcpy(srv->H.privkey.bytes,plugin->mypriv,sizeof(bits256));
        memcpy(srv->H.pubkey.bytes,plugin->mypub,sizeof(bits256));
    }
    return(tp);
}

int32_t pangea_start(struct plugin_info *plugin,char *retbuf,char *base,uint32_t timestamp,uint64_t bigblind,uint64_t ante,int32_t rakemillis,int32_t maxplayers,uint32_t minbuyin,uint32_t maxbuyin,cJSON *json)
{
    char *addrstr,*ciphers,*playerpubs,*isbotstr,destNXT[64]; struct pangea_thread *tp; struct cards777_pubdata *dp;
    int32_t createdflag,addrtype,haspubkey,i,j,slot,n,myind=-1,r,num=0,threadid=0; uint64_t addrs[512],isbot[512],tmp;
    uint8_t p2shtype; struct pangea_info *sp; cJSON *bids,*walletitem,*item;
    memset(addrs,0,sizeof(addrs));
    //memset(balances,0,sizeof(balances));
    pangea_buyins(&minbuyin,&maxbuyin);
    if ( rakemillis < 0 || rakemillis > PANGEA_MAX_HOSTRAKE )
    {
        printf("illegal rakemillis.%d\n",rakemillis);
        strcpy(retbuf,"{\"error\":\"illegal rakemillis\"}");
        return(-1);
    }
    if ( bigblind == 0 )
        bigblind = SATOSHIDEN;
    if ( (tp= THREADS[threadid]) == 0 )
    {
        pangea_threadinit(plugin,maxplayers);
        if ( (tp=THREADS[0]) == 0 )
        {
            strcpy(retbuf,"{\"error\":\"uinitialized threadid\"}");
            printf("%s\n",retbuf);
            return(-1);
        }
    }
    printf("mynxt64bits.%llu base.(%s) maxplayers.%d minbuyin.%u maxbuyin.%u\n",(long long)tp->nxt64bits,base,maxplayers,minbuyin,maxbuyin);
    if ( base == 0 || base[0] == 0 || maxplayers < 2 || maxplayers > CARDS777_MAXPLAYERS )
    {
        sprintf(retbuf,"{\"error\":\"bad params\"}");
        printf("%s\n",retbuf);
        return(-1);
    }
    addrtype = coin777_addrtype(&p2shtype,base);
    if ( (bids= jarray(&n,json,"bids")) != 0 )
    {
        printf("numbids.%d\n",n);
        for (i=num=0; i<n; i++)
        {
            item = jitem(bids,i);
            if ( (addrs[num]= j64bits(item,"offerNXT")) != 0 && (walletitem= jobj(item,"wallet")) != 0 )
            {
                if ( j64bits(walletitem,"bigblind") == bigblind && j64bits(walletitem,"ante") == ante && juint(walletitem,"rakemillis") == rakemillis )
                {
                    //balances[num] = j64bits(walletitem,"balance");
                    isbot[num] = juint(walletitem,"isbot");
                    printf("(i.%d %llu) ",i,(long long)addrs[num]);//,dstr(balances[num]));
                    for (j=0; j<num; j++)
                        if ( addrs[j] == addrs[num] )
                            break;
                    if ( j == num )
                    {
                        if ( addrs[num] == tp->nxt64bits )
                            myind = num;
                        printf("%llu ",(long long)addrs[num]);
                        num++;
                    }
                }
                else printf("%d: %llu mismatched walletitem bigblind %.8f ante %.8f rake %.1f%%\n",i,(long long)addrs[num],dstr(j64bits(walletitem,"bigblind")),dstr(j64bits(walletitem,"ante")),(double)juint(walletitem,"rakemillis")/10.);
            }
        }
    }
    printf("(%llu) pangea_start(%s) threadid.%d myind.%d num.%d maxplayers.%d\n",(long long)tp->nxt64bits,base,tp->threadid,myind,num,maxplayers);
    if ( (i= myind) > 0 )
    {
        addrs[i] = addrs[0];
        addrs[0] = tp->nxt64bits;
        //tmp = balances[i];
        //balances[i] = balances[0];
        //balances[0] = tmp;
        tmp = isbot[i];
        isbot[i] = isbot[0];
        isbot[0] = tmp;
        i = 0;
        strcpy(retbuf,"{\"error\":\"host needs to be locally started and the first entry in addrs\"}");
        return(-1);
    }
    while ( num > maxplayers )
    {
        r = (rand() % (num-1));
        printf("swap out %d of %d\n",r+1,num);
        num--;
        isbot[r + 1] = isbot[num];
        //balances[r + 1] = balances[num];
        addrs[r + 1] = addrs[num];
    }
    printf("pangea numplayers.%d\n",num);
    if ( (sp= pangea_create(tp,&createdflag,base,timestamp,addrs,num,bigblind,ante,isbot,minbuyin,maxbuyin)) == 0 )
    {
        printf("cant create table.(%s) numaddrs.%d\n",base,num);
        strcpy(retbuf,"{\"error\":\"cant create table, make sure all players have published NXT pubkeys\"}");
        return(-1);
    }
    printf("back from pangea_create\n");
    dp = sp->dp, dp->table = sp;
    sp->myind = myind;
    if ( createdflag != 0 && myind == 0 && addrs[myind] == tp->nxt64bits )
    {
        tp->numcards = dp->numcards, tp->N = dp->N, tp->M = dp->M;
        dp->rakemillis = juint(json,"rakemillis");
        if ( dp->rakemillis > PANGEA_MAX_HOSTRAKE )
            dp->rakemillis = PANGEA_MAX_HOSTRAKE;
        dp->rakemillis += PANGEA_MINRAKE_MILLIS;
        dp->minbuyin = minbuyin, dp->maxbuyin = maxbuyin;
        tp->hn.server->clients[myind].pubdata = dp;
        tp->hn.server->clients[myind].privdata = sp->priv;
        tp->hn.server->H.pubdata = dp;
        tp->hn.server->H.privdata = sp->priv;
        init_sharenrs(dp->hand.sharenrs,0,dp->N,dp->N);
        for (j=0; j<dp->N; j++)
        {
            if ( THREADS[j] != 0 )
                dp->playerpubs[j] = THREADS[j]->hn.client->H.pubkey;
            else
            {
                expand_nxt64bits(destNXT,addrs[j]);
                dp->playerpubs[j] = issue_getpubkey(&haspubkey,destNXT);
                if ( (slot= hostnet777_register(THREADS[0]->hn.server,dp->playerpubs[j],-1)) != j )
                    printf("unexpected register slot.%d for j.%d\n",slot,j);
            }
            //printf("thread[%d] pub.%llx priv.%llx\n",j,(long long)dp->playerpubs[j].txid,(long long)THREADS[j]->hn.client->H.privkey.txid);
        }
        isbotstr = jprint(addrs_jsonarray(isbot,num),1);
        //balancestr = jprint(addrs_jsonarray(balances,num),1);
        addrstr = jprint(addrs_jsonarray(addrs,num),1);
        ciphers = jprint(pangea_ciphersjson(dp,sp->priv),1);
        playerpubs = jprint(pangea_playerpubs(dp->playerpubs,num),1);
        dp->readymask |= (1 << sp->myind);
        sprintf(retbuf,"{\"cmd\":\"newtable\",\"broadcast\":\"allnodes\",\"myind\":%d,\"pangea_endpoint\":\"%s\",\"plugin\":\"relay\",\"destplugin\":\"pangea\",\"method\":\"busdata\",\"submethod\":\"newtable\",\"my64bits\":\"%llu\",\"tableid\":\"%llu\",\"timestamp\":%u,\"M\":%d,\"N\":%d,\"base\":\"%s\",\"bigblind\":\"%llu\",\"minbuyin\":\"%d\",\"maxbuyin\":\"%u\",\"rakemillis\":\"%u\",\"ante\":\"%llu\",\"playerpubs\":%s,\"addrs\":%s,\"isbot\":%s,\"millitime\":\"%lld\"}",sp->myind,tp->hn.server->ep.endpoint,(long long)tp->nxt64bits,(long long)sp->tableid,sp->timestamp,dp->M,dp->N,sp->base,(long long)bigblind,dp->minbuyin,dp->maxbuyin,dp->rakemillis,(long long)ante,playerpubs,addrstr,isbotstr,(long long)hostnet777_convmT(&tp->hn.server->H.mT,0)); //\"pluginrequest\":\"SuperNET\",
#ifdef BUNDLED
        {
            char *busdata_sync(uint32_t *noncep,char *jsonstr,char *broadcastmode,char *destNXTaddr);
            char *str; uint32_t nonce;
            if ( (str= busdata_sync(&nonce,retbuf,"allnodes",0)) != 0 )
                free(str);
        }
#endif
        printf("START.(%s)\n",retbuf);
        dp->pmworks |= (1 << sp->myind);
        free(addrstr), free(ciphers), free(playerpubs), free(isbotstr);// free(balancestr);
    }
    return(0);
}

void pangea_test(struct plugin_info *plugin)//,int32_t numthreads,int64_t bigblind,int64_t ante,int32_t rakemillis)
{
    char retbuf[65536]; bits256 privkey,pubkey; int32_t i,slot,threadid; struct pangea_thread *tp; struct hostnet777_client **clients;
    struct hostnet777_server *srv; cJSON *item,*bids,*walletitem,*testjson = cJSON_CreateObject();
    sleep(3);
    int32_t numthreads; int64_t bigblind,ante; int32_t rakemillis;
    numthreads = 9; bigblind = SATOSHIDEN; ante = SATOSHIDEN/10; rakemillis = PANGEA_MAX_HOSTRAKE;
    //plugin->sleepmillis = 1;
    if ( PANGEA_MAXTHREADS > 1 && PANGEA_MAXTHREADS <= 9 )
        numthreads = PANGEA_MAXTHREADS;
    else PANGEA_MAXTHREADS = numthreads;
    if ( plugin->transport[0] == 0 )
        strcpy(plugin->transport,"tcp");
    if ( plugin->ipaddr[0] == 0 )
        strcpy(plugin->ipaddr,"127.0.0.1");
    if ( plugin->pangeaport == 0 )
        plugin->pangeaport = 8888;
    //if ( portable_thread_create((void *)hostnet777_idler,hn) == 0 )
    //    printf("error launching server thread\n");
    if ( (clients= calloc(numthreads,sizeof(*clients))) == 0 )
    {
        printf("pangea_test: unexpected out of mem\n");
        return;
    }
    for (threadid=0; threadid<PANGEA_MAXTHREADS; threadid++)
    {
        if ( (tp= calloc(1,sizeof(*THREADS[threadid]))) == 0 )
        {
            printf("pangea_test: unexpected out of mem\n");
            return;
        }
        tp->threadid = threadid;
        tp->nxt64bits = conv_NXTpassword(privkey.bytes,pubkey.bytes,(void *)&threadid,sizeof(threadid));
        if ( threadid == 0 )
        {
            if ( (srv= hostnet777_server(privkey,pubkey,0,0,0,numthreads)) == 0 )
            {
                printf("cant create hostnet777 server\n");
                return;
            }
            tp->hn.server = srv;
            clients[0] = (void *)srv;
            slot = threadid;
           // srv->H.privkey = privkey, srv->H.pubkey = pubkey;
        }
        else
        {
            if ( (slot= hostnet777_register(srv,pubkey,-1)) >= 0 && slot == threadid )
            {
                if ( (clients[threadid]= hostnet777_client(privkey,pubkey,srv->ep.endpoint,slot)) == 0 )
                    printf("error creating clients[%d]\n",threadid);
                else
                {
                    tp->hn.client = clients[threadid];
                    //tp->hn.client->H.privkey = privkey, tp->hn.client->H.pubkey = pubkey;
                    //if ( portable_thread_create((void *)hostnet777_idler,hn) == 0 )
                    //    printf("error launching clients[%d] thread\n",threadid);
                }
            } else printf("error slot.%d != threadid.%d\n",slot,threadid);
        }
        printf("%llu: slot.%d client.%p -> %llu pubkey.%llx/%llx privkey.%llx/%llx\n",(long long)tp->nxt64bits,slot,clients[threadid],(long long)clients[threadid]->H.nxt64bits,(long long)clients[threadid]->H.pubkey.txid,(long long)pubkey.txid,(long long)clients[threadid]->H.privkey.txid,(long long)privkey.txid);
        THREADS[threadid] = tp;
    }
    bids = cJSON_CreateArray();
    for (i=0; i<numthreads; i++)
    {
        item = cJSON_CreateObject();
        walletitem = cJSON_CreateObject();
        //if ( i != 0 )
            jaddnum(walletitem,"isbot",1);
        jadd64bits(walletitem,"bigblind",bigblind);
        jadd64bits(walletitem,"ante",ante);
        jaddnum(walletitem,"rakemillis",rakemillis);
        //jadd64bits(walletitem,"balance",bigblind * 100);
        jadd64bits(item,"offerNXT",THREADS[i]->nxt64bits);
        jadd(item,"wallet",walletitem);
        jaddi(bids,item);
    }
    jadd(testjson,"bids",bids);
    jadd64bits(testjson,"offerNXT",THREADS[0]->nxt64bits);
    jadd64bits(testjson,"bigblind",bigblind);
    jadd64bits(testjson,"ante",ante);
    jaddnum(testjson,"rakemillis",rakemillis);
    printf("TEST.(%s)\n",jprint(testjson,0));
    pangea_start(plugin,retbuf,"BTCD",0,bigblind,ante,rakemillis,i,0,0,testjson);
    free_json(testjson);
    testjson = cJSON_Parse(retbuf);
    //printf("BROADCAST.(%s)\n",retbuf);
    for (threadid=1; threadid<numthreads; threadid++)
        pangea_newtable(threadid,testjson,THREADS[threadid]->nxt64bits,THREADS[threadid]->hn.client->H.privkey,THREADS[threadid]->hn.client->H.pubkey,0,0,0,0,0);
    tp = THREADS[0];
    pangea_newdeck(&tp->hn);
}

char *pangea_buyin(uint64_t my64bits,uint64_t tableid,cJSON *json)
{
    struct pangea_info *sp; uint32_t buyin; uint64_t amount = 0; char hex[1024];
    if ( (sp= pangea_find64(tableid,my64bits)) != 0 && sp->dp != 0 && sp->tp != 0 && (amount= j64bits(json,"amount")) != 0 )
    {
        buyin = (uint32_t)(amount / sp->dp->bigblind);
        printf("buyin.%u amount %.8f -> %.8f\n",buyin,dstr(amount),dstr(buyin * sp->bigblind));
        if ( buyin >= sp->dp->minbuyin && buyin <= sp->dp->maxbuyin )
        {
            sp->dp->balances[sp->myind] = amount;
            pangea_sendcmd(hex,&sp->tp->hn,"addfunds",-1,(void *)&amount,sizeof(amount),sp->myind,-1);
            pangea_sendcmd(hex,&sp->tp->hn,"addfunds",0,(void *)&amount,sizeof(amount),sp->myind,-1);
            return(clonestr("{\"result\":\"buyin sent\"}"));
        }
        else
        {
            printf("buyin.%d vs (%d %d)\n",buyin,sp->dp->minbuyin,sp->dp->maxbuyin);
            return(clonestr("{\"error\":\"buyin too small or too big\"}"));
        }
    }
    return(clonestr("{\"error\":\"cant buyin unless you are part of the table\"}"));
}

char *pangea_mode(uint64_t my64bits,uint64_t tableid,cJSON *json)
{
    struct pangea_info *sp; char *chatstr,hex[8192]; int32_t i; uint64_t pm;
    if ( jobj(json,"autoshow") != 0 )
    {
        if ( tableid == 0 )
            Showmode = juint(json,"autoshow");
        else if ( (sp= pangea_find64(tableid,my64bits)) != 0 && sp->priv != 0 )
            sp->priv->autoshow = juint(json,"autoshow");
        else return(clonestr("{\"error\":\"autoshow not tableid or sp->priv\"}"));
        return(clonestr("{\"result\":\"set autoshow mode\"}"));
    }
    else if ( jobj(json,"autofold") != 0 )
    {
        if ( tableid == 0 )
            Autofold = juint(json,"autofold");
        else if ( (sp= pangea_find64(tableid,my64bits)) != 0 && sp->priv != 0 )
            sp->priv->autofold = juint(json,"autofold");
        else return(clonestr("{\"error\":\"autofold not tableid or sp->priv\"}"));
        return(clonestr("{\"result\":\"set autofold mode\"}"));
    }
    else if ( (sp= pangea_find64(tableid,my64bits)) != 0 && (chatstr= jstr(json,"chat")) != 0 && strlen(chatstr) < 256 )
    {
        if ( (pm= j64bits(json,"pm")) != 0 )
        {
            for (i=0; i<sp->numaddrs; i++)
                if ( sp->addrs[i] == pm )
                    break;
            if ( i == sp->numaddrs )
                return(clonestr("{\"error\":\"specified pm destination not at table\"}"));
        } else i = -1;
        pangea_sendcmd(hex,&sp->tp->hn,"chat",i,(void *)chatstr,(int32_t)strlen(chatstr)+1,sp->myind,-1);
    }
    return(clonestr("{\"error\":\"unknown pangea mode\"}"));
}

char *pangea_univ(uint8_t *mypriv,cJSON *json)
{
    char *addrtypes[][3] = { {"BTC","0","80"}, {"LTC","48"}, {"BTCD","60","bc"}, {"DOGE","30"}, {"VRC","70"}, {"OPAL","115"}, {"BITS","25"} };
    //int32_t getprivkey(uint8_t privkey[32],char *name,char *coinaddr);
    char *wipstr,*coin,*coinaddr,pubkeystr[67],rsaddr[64],destaddr[64],wifbuf[128]; uint8_t priv[32],pub[33],addrtype; int32_t i;
    uint64_t nxt64bits; cJSON *retjson,*item;
    if ( (coin= jstr(json,"coin")) != 0 )
    {
        if ( (wipstr= jstr(json,"wif")) != 0 || (wipstr= jstr(json,"wip")) != 0 )
        {
            printf("got wip.(%s)\n",wipstr);
            btc_wip2priv(priv,wipstr);
        }
        else if ( (coinaddr= jstr(json,"addr")) != 0 )
        {
            if ( getprivkey(priv,coin,coinaddr) < 0 )
                return(clonestr("{\"error\":\"cant get privkey\"}"));
        }
    } else memcpy(priv,mypriv,sizeof(priv));
    btc_priv2pub(pub,priv);
    init_hexbytes_noT(pubkeystr,pub,33);
    printf("pubkey.%s\n",pubkeystr);
    retjson = cJSON_CreateObject();
    jaddstr(retjson,"btcpubkey",pubkeystr);
    for (i=0; i<sizeof(addrtypes)/sizeof(*addrtypes); i++)
    {
        if ( btc_coinaddr(destaddr,atoi(addrtypes[i][1]),pubkeystr) == 0 )
        {
            item = cJSON_CreateObject();
            jaddstr(item,"addr",destaddr);
            if ( addrtypes[i][2] != 0 )
            {
                decode_hex(&addrtype,1,addrtypes[i][2]);
                btc_priv2wip(wifbuf,priv,addrtype);
                jaddstr(item,"wif",wifbuf);
            }
            jadd(retjson,addrtypes[i][0],item);
        }
    }
    nxt64bits = nxt_priv2addr(rsaddr,pubkeystr,priv);
    item = cJSON_CreateObject();
    jaddstr(item,"addressRS",rsaddr);
    jadd64bits(item,"address",nxt64bits);
    jaddstr(item,"pubkey",pubkeystr);
    jadd(retjson,"NXT",item);
    return(jprint(retjson,1));
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*base,*retstr = 0; int32_t maxplayers; cJSON *argjson;
    retbuf[0] = 0;
    fprintf(stderr,"<<<<<<<<<<<< INSIDE PANGEA! process %s (%s) forwarder.(%s) sender.(%s)\n",plugin->name,jsonstr,forwarder,sender);
    if ( initflag > 0 )
    {
        uint64_t conv_NXTpassword(unsigned char *mysecret,unsigned char *mypublic,uint8_t *pass,int32_t passlen);
        //PANGEA.readyflag = 1;
        plugin->sleepmillis = 100;
        plugin->allowremote = 1;
        argjson = cJSON_Parse(jsonstr);
        plugin->nxt64bits = set_account_NXTSECRET(plugin->mypriv,plugin->mypub,plugin->NXTACCT,plugin->NXTADDR,plugin->NXTACCTSECRET,sizeof(plugin->NXTACCTSECRET),argjson,0,0,0);
        free_json(argjson);
        printf("my64bits %llu ipaddr.%s mypriv.%02x mypub.%02x\n",(long long)plugin->nxt64bits,plugin->ipaddr,plugin->mypriv[0],plugin->mypub[0]);
        if ( (PANGEA_MAXTHREADS= juint(json,"pangea_loopback")) != 0 )
            portable_thread_create((void *)pangea_test,plugin);//,9,SATOSHIDEN,SATOSHIDEN/10,10);
        printf("initialized PANGEA\n");
        if ( 0 )
        {
            int32_t i; char str[8];
            for (i=0; i<52; i++)
            {
                cardstr(str,i);
                printf("(%d %s) ",i,str);
            }
            printf("cards\n");
        }
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        methodstr = jstr(json,"method");
        resultstr = jstr(json,"result");
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        else if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"newtable") == 0 )
            retstr = pangea_newtable(juint(json,"threadid"),json,plugin->nxt64bits,*(bits256 *)plugin->mypriv,*(bits256 *)plugin->mypub,plugin->transport,plugin->ipaddr,plugin->pangeaport,juint(json,"minbuyin"),juint(json,"maxbuyin"));
        else if ( sender == 0 || sender[0] == 0 )
        {
            if ( strcmp(methodstr,"start") == 0 )
            {
                strcpy(retbuf,"{\"result\":\"start issued\"}");
                if ( (base= jstr(json,"base")) != 0 )
                {
                    if ( (maxplayers= juint(json,"maxplayers")) < 2 )
                        maxplayers = 2;
                    else if ( maxplayers > CARDS777_MAXPLAYERS )
                        maxplayers = CARDS777_MAXPLAYERS;
                    if ( jstr(json,"resubmit") == 0 )
                        sprintf(retbuf,"{\"resubmit\":[{\"method\":\"start\"}, {\"bigblind\":\"%llu\"}, {\"ante\":\"%llu\"}, {\"rakemillis\":\"%u\"}, {\"maxplayers\":%d}, {\"minbuyin\":%d}, {\"maxbuyin\":%d}],\"pluginrequest\":\"SuperNET\",\"plugin\":\"InstantDEX\",\"method\":\"orderbook\",\"base\":\"BTCD\",\"exchange\":\"pangea\",\"allfields\":1}",(long long)j64bits(json,"bigblind"),(long long)j64bits(json,"ante"),juint(json,"rakemillis"),maxplayers,juint(json,"minbuyin"),juint(json,"maxbuyin"));
                    else if ( pangea_start(plugin,retbuf,base,0,j64bits(json,"bigblind"),j64bits(json,"ante"),juint(json,"rakemillis"),maxplayers,juint(json,"minbuyin"),juint(json,"maxbuyin"),json) < 0 )
                        ;
                } else strcpy(retbuf,"{\"error\":\"no base specified\"}");
            }
            else if ( strcmp(methodstr,"status") == 0 )
                retstr = pangea_status(plugin->nxt64bits,j64bits(json,"tableid"),json);
        }
        //else if ( strcmp(methodstr,"turn") == 0 )
        //    retstr = pangea_input(plugin->nxt64bits,j64bits(json,"tableid"),json);
    }
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

char *Pangea_bypass(uint64_t my64bits,uint8_t myprivkey[32],cJSON *json)
{
    char *methodstr,*retstr = 0;
    if ( (methodstr= jstr(json,"method")) != 0 && (strcmp(methodstr,"turn") == 0 || strcmp(methodstr,"status") == 0 || strcmp(methodstr,"rosetta") == 0 || strcmp(methodstr,"rates") == 0 || strcmp(methodstr,"buyin") == 0|| strcmp(methodstr,"mode") == 0) )
    {
        if ( strcmp(methodstr,"turn") == 0 )
            retstr = pangea_input(my64bits,j64bits(json,"tableid"),json);
        else if ( strcmp(methodstr,"status") == 0 )
            retstr = pangea_status(my64bits,j64bits(json,"tableid"),json);
        else if ( strcmp(methodstr,"mode") == 0 )
            retstr = pangea_mode(my64bits,j64bits(json,"tableid"),json);
        else if ( strcmp(methodstr,"rosetta") == 0 )
            retstr = pangea_univ(myprivkey,json);
        else if ( strcmp(methodstr,"buyin") == 0 )
            retstr = pangea_buyin(my64bits,j64bits(json,"tableid"),json);
        else if ( strcmp(methodstr,"rates") == 0 )
            retstr = peggyrates(0,jstr(json,"name"));
    }
    return(retstr);
}

#include "../agents/plugin777.c"
