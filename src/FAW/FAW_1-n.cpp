/**
 * FAW 1-n simulation
 * version 1
 * Zhu-20211020 created
 * 20211026 0:58 completed
 * 1104 fix n>9
 * 1105 function name optimize  output all reward
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

//0 BTC 1 ETH
#define BTC 0
#define ETH 1
int typ = BTC;

double vp[2][12] = {0.15,0.13,0.1,0.1,0.07,0.07,0.06,0.02,0.015,0.015,0.01,0.01,
                0.25,0.1,0.05,0.035,0.03,0.03,0.025,0.025,0.02,0.02,0.015,0.015};

double t_min_BTC = 0.0, t_min_ETH = 0.0;
double t_max_BTC = 0.2, t_max_ETH = 0.2;
double t_dlt = 0.005;

///number of victims
const int nv = 5;
//max element of array
const int nM = 2 * nv + 3;
//propagation delay 200~400ms in BTC 10min 300~477ms in ETH 13.4s
double theta = typ == BTC ? 0.0005 : 0.025;
//name of miners mp op vh im
char minername[nM][5];

//computing power
//attacker, MP, IMs, VHs, OPs
double att, mp, op, im[nv], vh[nv], c = 0.5;
double tau[nv];

///max block level
int MAX_LEVEL = 500;

int tmpmax=-999;
double tmpmin = 999.0;

/**区块链结构体**/
typedef struct Block_NODE {
	int block_num;
	int block_level;
	//m, i, v, h
	char miner[5];
	//withhold=1, publish=0
	int ishold;
	int if_mainchain;
	//0-2
	int reference_num;
	//0 no, 1 yes
	int be_referenced_flag;
	struct Block_NODE *next[nv+2], *prev;
} Block, *BlockList;

/**区块链创建函数
 * 返回创世块
 **/
BlockList BlockListCreate(){
	BlockList head = (Block*) malloc(sizeof(Block));
	head->block_num = 0;
	head->block_level = 0;
	strcpy(head->miner, "op");
	for(int i = 0; i < nv+2; i ++)
        head->next[i] = NULL;
	head->prev = NULL;
	head->if_mainchain = 1;
	return head;
}

/**区块创建函数
 * 输入块号 高度 矿工
 * 返回区块地址
 **/
Block* BlockCreate(int block_num, int block_level, char miner[]){
    Block* p = (Block*)malloc(sizeof(Block));
	p->block_num = block_num;
	p->block_level = block_level;
	strcpy(p->miner, miner);
	p->ishold = 0;
	p->if_mainchain = 0;
	//0-2
	p->reference_num = 0;
	//0 no, 1 yes
	p->be_referenced_flag = 0;
	for(int i = 0; i < nv+2; i ++)
        p->next[i] = NULL;
	p->prev = NULL;
	return p;
}

/**区块上链
 * 输入区块链，待上链区块
 * 返回区块链尾
 **/
BlockList BlockInsert(BlockList pb, Block* b){
    for(int i = 0; i < nv+2; i ++)
    {
        if(pb->next[i] == NULL)
        {
            pb->next[i] = b;
            b->prev = pb;
            pb = pb->next[i];
            return pb;
        }
    }
    printf("error, all next pointers are full!\nblocknum:%d,blocklevel:%d,miner:%s\n",b->block_num,b->block_level,b->miner);
	return pb;
}

/**释放区块
 * 输入尾块
 * 释放主链区块
**/
void ReleaseBlock(Block *b){
    Block *p = b, *pp = NULL;
    while(p->prev != NULL)
    {
        pp = p->prev;
        for(int i = 0; i < nv+2; i ++)
        {
            if(p->next[i] != NULL)
                free(p->next[i]);
        }
        p = pp;
    }
}

/**显示主链区块
 * 输入创世块
 **/
void Display_BlockChain(BlockList Gen_b){
    int havemain = 1;
    Block *p = Gen_b, *pn = NULL;
    printf("*%d,%d,%s\n", p->block_num,p->block_level,p->miner);
    while(p->block_level < MAX_LEVEL)
    {
        havemain = 0;
        for(int i = 0; i < nv+2; i ++)
        {
            if(p->next[i] != NULL)
            {
                if(p->next[i]->if_mainchain)
                {
                    havemain = 1;
                    pn = p->next[i];
                    printf("*");
                }
                else if(p->next[i]->ishold == 1)
                    printf("-");
                else printf(" ");
                printf("%d,%d,%s\t", p->next[i]->block_num,p->next[i]->block_level,p->next[i]->miner);
            }
        }
        if(!havemain){
            printf("break!\n\n");
            break;
        }
        printf("\n");
        p = pn;
    }
}

//生成1个指数分布样本
double GenExp(double lambda){
	double pv = 0.0;
	while(pv - 0 < 1e-6)
        pv = (double)rand()/RAND_MAX;
	pv = (-1  / lambda)*log(1-pv);
	return pv;
}

//生成随机整数
int RandNum(int d){
    int x = 0;
    x = rand() % d + 1;
    return x;
}

int ArgMax(int a[], int len){
    int index = 0;
    tmpmax = -999;
    for(int i = 0; i < len; i ++)
    {
        if(a[i] > tmpmax)
        {
            tmpmax = a[i];
            index = i;
        }
    }
    return index;
}

int ArgMin(double a[], int len){
    int index = 0;
    tmpmin = 999.0;
    for(int i = 0; i < len; i ++)
    {
        if(a[i] < tmpmin)
        {
            tmpmin = a[i];
            index = i;
        }
    }
    return index;
}

/**获取VP编号
 * 输入minername
 * 输出VP编号 1~nv
 **/
int GetVPnum(char m[]){
    int num = 0, i = 2;
    char st[5];
    strcpy(st,m);
    while(st[i] != '\0')
    {
        num += st[i] - '0';
        num = num * 10;
        i ++;
    }
    num /= 10;
    return num;
}

/**参数初始化-给定tau
 * 输入attacker总算力，vp分算力，im分配比例
 * att, mp, im[nv], vh[nv], op，tau[nv]
 * 0 ok, 1 vp over, 2 tau over
**/
int InitParam_fixtau(double a, double t[], double v[]){
    int err = 0;
    double vhsum = 0.0, tausum = 0.0;
    att = a;
    //computing power
    for(int i = 0; i < nv; i++)
    {
        vh[i] = v[i];
        tau[i] = t[i];
        im[i] = a * t[i];

        vhsum += v[i];
        tausum += t[i];
        if(vhsum >= 1)
        {
            printf("VP set error! SUM_VP=%lf\n", vhsum);
            err = 1;
            break;
        }
        if(tausum >= 1)
        {
            printf("Tau set error! SUM_TAU=%lf\n", tausum);
            err = 2;
            break;
        }
    }
    if(!err)
    {
        mp = a * (1-tausum);
        op = 1 - a - vhsum;
    }
    sprintf(minername[0],"mp");
    sprintf(minername[1],"op");
    sprintf(minername[2],"op");
    for(int i = 0; i < nv; i ++)
    {
        sprintf(minername[i+3],"vh%d",i+1);
        sprintf(minername[i+3+nv],"im%d",i+1);
    }
    return err;
}

/**参数初始化-占比分配tau
 * 输入attacker总算力，总tau值
 * att, mp, im[nv], vh[nv], op，tau[nv]
 * 0 ok, 1 vp over, 2 tau over
**/
void InitParam_proptau(double a, double t){
    int i = 0;
    double vhsum = 0.0;
    att = a;
    //vp总算力
    for(i = 0; i < nv; i ++)
        vhsum += vp[typ][i];
    //computing power
    for(int i = 0; i < nv; i++)
    {
        vh[i] = vp[typ][i];
        tau[i] = t * vp[typ][i] / vhsum;
        im[i] = a * tau[i];
    }
    mp = a * (1-t);
    op = 1 - a - vhsum;
    sprintf(minername[0],"mp");
    sprintf(minername[1],"op");
    sprintf(minername[2],"op");
    for(int i = 0; i < nv; i ++)
    {
        sprintf(minername[i+3],"vh%d",i+1);
        sprintf(minername[i+3+nv],"im%d",i+1);
    }
}

/**参数初始化-平分tau
 * 输入attacker总算力，总tau值
 * att, mp, im[nv], vh[nv], op，tau[nv]
 * 0 ok, 1 vp over, 2 tau over
**/
void InitParam_equaltau(double a, double t){
    int i = 0;
    double vhsum = 0.0;
    att = a;
    //vp总算力
    for(i = 0; i < nv; i ++)
        vhsum += vp[typ][i];
    //computing power
    for(int i = 0; i < nv; i++)
    {
        vh[i] = vp[typ][i];
        tau[i] = t / nv;
        im[i] = a * tau[i];
    }
    mp = a * (1-t);
    op = 1 - a - vhsum;
    sprintf(minername[0],"mp");
    sprintf(minername[1],"op");
    sprintf(minername[2],"op");
    for(int i = 0; i < nv; i ++)
    {
        sprintf(minername[i+3],"vh%d",i+1);
        sprintf(minername[i+3+nv],"im%d",i+1);
    }
}

/**输出系统参数**/
void ShowParam(){
    printf("\n******** System Parameters **********\n");
    printf("\n----  Fork Probability ----\n");
    printf("theta=%lf\n", theta);
    printf("Max Level=%d\n", MAX_LEVEL);
    printf("\n--- Computational Power ---\n");
    printf("malicious miners: %.2lf\n", att);
    printf("MP:%-5.3lf\nOPs:%-4.2lf\n\n", mp, op);
    for(int i = 0;i < nv; i ++)
    {
        printf("VH-%d: %-5.3lf\n", i, vh[i]);
        printf("IM-%d: %-5.3lf\t tau: %-5.3lf\n\n", i, im[i], tau[i]);
    }
    printf("*************************************\n");
}


/**区块链分叉结构体
 * miners每个矿工出块情况 出块则置1
 * type当前区块链分叉类型 -1 disable
 * type0 m 1 o 2 v 3 i 4
 * type1 MO 3 MV 5 OV 6 MI 9 OI 10 VI 12
 * type2 各生成2块 O2 1 V2 2 I2 4
 **/
struct FORK{
    int blocknum = 0;
    int miners[2*nv+3]={0,};
    //mp 1 op 2 vh 3 im 4
    int type0 = 0;
    //MO 3 MV 5 OV 6 MI 9 OI 10 VI 12
    int type1 = 0;
    //O2 1 V2 2 I2 4
    int type2 = 0;
}fork, fork_tmp;

/**重置fork变量**/
void ResetFork(){
    for(int i = 0; i < nM; i ++)
        fork.miners[i] = 0;
    fork.type0 = -1;
    fork.type1 = -1;
    fork.type2 = -1;
    fork.blocknum = 0;
}

/**显示fork变量**/
void ShowFork(){
    printf("blocknum:%d\n",fork.blocknum);
    for(int i = 0; i < nM; i ++)
        printf("%d ", fork.miners[i]);
    printf("\n%d %d %d\n", fork.type0, fork.type1, fork.type2);
}

//矿工链 mp op1 op2 vp1~n im1~n
Block *p_b[nM];
//当前块 后nv块为im[i]
Block *c_b[nv+2] = {NULL,};

/**区块生成事件
 * 返回分叉情况
 **/
void GenBlocks(){
    //下轮出块时间
	double emp, eop1, eop2, evh[nv], eim[nv];
	double ft1, ft2;
	//double r1 = 1-2*theta;
	int slt[2] = {0,0};

	int i = 0;

	//保留前轮信息 重置当前信息
	ResetFork();

    //生成出块时间
    emp = GenExp(mp);
    eop1 = GenExp(op*c);
    eop2 = GenExp(op*(1-c));
    for(i = 0; i < nv; i ++)
    {
        eim[i] = GenExp(im[i]);
        evh[i] = GenExp(vh[i]);
    }
	//事件时间数组
	double ent[nM];
    ent[0] = emp;
    ent[1] = eop1;
    ent[2] = eop2;
    for(i = 0; i < nv; i ++)
    {
        ent[i+3] = evh[i];
        ent[i+3+nv] = eim[i];
    }
    //寻找最早事件，确定出块事件
    //查找最早块
    slt[0] = ArgMin(ent, nM);
    ft1 = ent[slt[0]];
    //查找其次块
    ent[slt[0]] = 999.0;
    slt[1] = ArgMin(ent, nM);
    ft2 = ent[slt[1]];
    if(slt[0] == slt[1])
    {
        printf("\nEVENT WRONG! same time\n");
        return;
    }
    //判定是否同时
    //同时出块
    if(ft2 - ft1 <= theta)
    {
        int type_tmp = 0;
        for(i = 0; i < 2; i ++)
        {
            fork.miners[slt[i]] = 1;
            //mp
            if(slt[i] == 0)
                type_tmp += 1;
            //oa ob
            else if(slt[i] < 3)
                type_tmp += 2;
            //vh
            else if(slt[i] < nv+3)
                type_tmp += 4;
            //im
            else if(slt[i] < nM)
                type_tmp += 8;
            else
                printf("EVENT TYPE1 WRONG!\n");

            if(type_tmp == 0)
                printf("EVENT WRONG! no event\n");
            //同类池出两块
            else if(type_tmp % 4 == 0 && type_tmp != 12)
            {
                fork.type0 = -1;
                fork.type1 = -1;
                fork.type2 = type_tmp / 4;
            }
            else
            {
                fork.type0 = -1;
                fork.type1 = type_tmp;
                fork.type2 = -1;
            }
            fork.blocknum = 2;
        }
    }
    //只出一块
    else
    {
        fork.blocknum = 1;
        fork.type1 = 0;
        fork.type2 = -1;
        fork.miners[slt[0]] = 1;
        //mp
        if(slt[0] == 0)
            fork.type0 = 1;
        //op
        else if(slt[0] < 3)
            fork.type0 = 2;
        //vh
        else if(slt[0] < nv+3)
            fork.type0 = 3;
        //im
        else
            fork.type0 = 4;
    }
    fork_tmp = fork;
}

/**仿真主函数
 * 输入创世块
 * 返回末尾块
 **/
Block* Simulation(BlockList Gen_b){
	//IM持有数量
	int hold = 0;
	//区块高度
	int levb = 1;
	int numb = 0;
	//选择时的随机数
	int slt = 0, i = 0;
	//初始化区块链指针
	for(i = 0; i < nM; i ++)
        p_b[i] = Gen_b;


    srand(time(NULL));

	while(levb <= MAX_LEVEL){

        ///事件产生
        GenBlocks();
        //printf("\nlevel:%d\n",levb);
        //ShowFork();

        ///区块生成
        while(fork_tmp.blocknum > 0)
        {
            for(i = nM-1; i >= 0; i --)
            {
                if(fork_tmp.miners[i] == 1)
                {
                    //IM
                    if(nv+2 < i && i < nM)
                    {
                        //hold
                        if(fork_tmp.type1 == 0 || fork_tmp.type2 == 4)
                        {
                            if(!c_b[i-nv-1])
                            {
                                c_b[i-nv-1] = BlockCreate(numb+fork_tmp.blocknum, levb, minername[i]);
                                BlockInsert(p_b[i],c_b[i-nv-1]);
                                hold ++;
                                c_b[i-nv-1]->ishold = 1;
                            }
                        }
                        //io
                        else if(fork_tmp.type1 == 10)
                        {
                            c_b[fork_tmp.blocknum-1] = BlockCreate(numb+fork_tmp.blocknum, levb, minername[i]);
                            BlockInsert(p_b[i],c_b[fork_tmp.blocknum-1]);
                        }
                    }
                    //not IM
                    else
                    {
                        c_b[fork_tmp.blocknum-1] = BlockCreate(numb+fork_tmp.blocknum, levb, minername[i]);
                        BlockInsert(p_b[i],c_b[fork_tmp.blocknum-1]);
                    }
                    fork_tmp.miners[i] = 0;
                    fork_tmp.blocknum --;
                    break;
                }
            }
        }
        numb += fork.blocknum;

	    ///主链选择
        //无扣块
        if(!hold){
            switch(fork.type1){
            //no fork, consensus
            case 0: case 9: case 12:{
                for(i = 0; i < nM; i ++)
                    p_b[i] = c_b[0];
                break;
            }
            //mp op
            case 3:{
                //mp
                p_b[0] = c_b[0];
                //op1 op2
                for(i = 1; i < 3; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[i] = c_b[0];
                    else
                        p_b[i] = c_b[1];
                }
                //vp 1-n
                for(i = 0; i < nv; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                    {
                        p_b[i+3] = c_b[0];
                        p_b[i+nv+3] = c_b[0];
                    }
                    else
                    {
                        p_b[i+3] = c_b[1];
                        p_b[i+nv+3] = c_b[1];
                    }
                }
                break;
            }
            //mp vp cb[0] cb[1]
            case 5:{
                //mp
                p_b[0] = c_b[0];
                //op1 op2
                for(i = 1; i < 3; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[i] = c_b[0];
                    else
                        p_b[i] = c_b[1];
                }
                //vp 1-n
                for(i = 0; i < nv; i ++)
                {
                    //是这个vp出块
                    if(fork.miners[i+3])
                    {
                        p_b[i+3] = c_b[1];
                        p_b[i+nv+3] = c_b[1];
                    }
                    //不是这个vp出块
                    else
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                        {
                            p_b[i+3] = c_b[0];
                            p_b[i+nv+3] = c_b[0];
                        }
                        else
                        {
                            p_b[i+3] = c_b[1];
                            p_b[i+nv+3] = c_b[1];
                        }
                    }
                }
                break;
            }
            //vp op
            case 6:{
                //mp
                slt = RandNum(2);
                if(slt == 1)
                    p_b[0] = c_b[0];
                else
                    p_b[0] = c_b[1];
                //op1 op2
                for(i = 1; i < 3; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[i] = c_b[0];
                    else
                        p_b[i] = c_b[1];
                }
                //vp 1-n
                for(i = 0; i < nv; i ++)
                {
                    //是这个vp出块
                    if(fork.miners[i+3])
                    {
                        p_b[i+3] = c_b[1];
                        p_b[i+nv+3] = c_b[1];
                    }
                    //不是这个vp出块
                    else
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                        {
                            p_b[i+3] = c_b[0];
                            p_b[i+nv+3] = c_b[0];
                        }
                        else
                        {
                            p_b[i+3] = c_b[1];
                            p_b[i+nv+3] = c_b[1];
                        }
                    }
                }
                break;
            }
            //im op
            case 10:{
                //mp
                slt = RandNum(2);
                if(slt == 1)
                    p_b[0] = c_b[0];
                else
                    p_b[0] = c_b[1];
                //op1 op2
                for(i = 1; i < 3; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[i] = c_b[0];
                    else
                        p_b[i] = c_b[1];
                }
                //vp 1-n
                for(i = 0; i < nv; i ++)
                {
                    //是这个im出块
                    if(fork.miners[i+nv+3])
                    {
                        p_b[i+nv+3] = c_b[1];
                        p_b[i+3] = c_b[1];
                    }
                    //不是这个im出块
                    else
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                        {
                            p_b[i+nv+3] = c_b[0];
                            p_b[i+3] = c_b[0];
                        }
                        else
                        {
                            p_b[i+nv+3] = c_b[1];
                            p_b[i+3] = c_b[1];
                        }
                    }
                }
                break;
            }
            //v2 o2
            case -1:{
                switch(fork.type2)
                {
                //o2
                case 1:{
                    //mp
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[0] = c_b[0];
                    else
                        p_b[0] = c_b[1];
                    //op1 op2
                    for(i = 1; i < 3; i ++)
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                            p_b[i] = c_b[0];
                        else
                            p_b[i] = c_b[1];
                    }
                    //vp 1-n
                    for(i = 0; i < nv; i ++)
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                        {
                            p_b[i+3] = c_b[0];
                            p_b[i+nv+3] = c_b[0];
                        }
                        else
                        {
                            p_b[i+3] = c_b[1];
                            p_b[i+nv+3] = c_b[1];
                        }
                    }
                    break;
                }
                //v2
                case 2:{
                    //mp
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[0] = c_b[0];
                    else
                        p_b[0] = c_b[1];
                    //op1 op2
                    for(i = 1; i < 3; i ++)
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                            p_b[i] = c_b[0];
                        else
                            p_b[i] = c_b[1];
                    }
                    //vp 1-n
                    int tmp = 0;
                    for(i = 0; i < nv; i ++)
                    {
                        //是这个vp出块
                        if(fork.miners[i+3])
                        {
                            p_b[i+3] = c_b[tmp];
                            p_b[i+nv+3] = c_b[tmp];
                            tmp++;
                        }
                        //不是这个vp出块
                        else
                        {
                            slt = RandNum(2);
                            if(slt == 1)
                            {
                                p_b[i+3] = c_b[0];
                                p_b[i+nv+3] = c_b[0];
                            }
                            else
                            {
                                p_b[i+3] = c_b[1];
                                p_b[i+nv+3] = c_b[1];
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
                }
                break;
            }
            default:
                printf("Chain Choose ERROR 1!\n");
                return Gen_b;
            }
            for(i = 0; i < nv+2; i ++)
                c_b[i] = NULL;
            levb ++;
        }
        //扣块
        else{
            switch(fork.type1){
            //con
            case 0:{
                //op
                if(fork.type0 == 2)
                {
                    int cnt = 0;
                    Block* slt_b[nv]={NULL,};
                    //vp
                    for(i = 2; i < nv+2; i ++)
                    {
                        if(c_b[i] != NULL)
                        {
                            c_b[i]->ishold = 0;
                            p_b[i+1] = c_b[i];
                            p_b[i+1+nv] = c_b[i];
                            slt_b[cnt] = c_b[i];
                            cnt ++;
                        }
                    }
                    for(i = 2; i < nv+2; i ++)
                    {
                        if(c_b[i] == NULL)
                        {
                            slt = RandNum(cnt+1);
                            if(slt == cnt+1)
                            {
                                p_b[i+1] = c_b[0];
                                p_b[i+1+nv] = c_b[0];
                            }
                            else
                            {
                                p_b[i+1] = slt_b[slt-1];
                                p_b[i+1+nv] = slt_b[slt-1];
                            }
                        }
                    }
                    //mp op1 op2
                    for(i = 0; i < 3; i ++)
                    {
                        slt = RandNum(cnt+1);
                        if(slt == cnt+1)
                            p_b[i] = c_b[0];
                        else
                            p_b[i] = slt_b[slt-1];
                    }
                }
                //not im
                else if(fork.type0 != 4)
                {
                    for(i = 0; i < nM; i ++)
                        p_b[i] = c_b[0];
                }
                break;
            }
            //mi iv
            case 9: case 12:{
                for(i = 0; i < nM; i ++)
                    p_b[i] = c_b[0];
                break;
            }
            //mp op
            case 3:{
                //mp
                p_b[0] = c_b[0];
                //op1 op2
                for(i = 1; i < 3; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[i] = c_b[0];
                    else
                        p_b[i] = c_b[1];
                }
                //vp 1-n
                for(i = 0; i < nv; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                    {
                        p_b[i+3] = c_b[0];
                        p_b[i+nv+3] = c_b[0];
                    }
                    else
                    {
                        p_b[i+3] = c_b[1];
                        p_b[i+nv+3] = c_b[1];
                    }
                }
                break;
            }
            //mp vp
            case 5:{
                //mp
                p_b[0] = c_b[0];
                //op1 op2
                for(i = 1; i < 3; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[i] = c_b[0];
                    else
                        p_b[i] = c_b[1];
                }
                //vp 1-n
                for(i = 0; i < nv; i ++)
                {
                    //是这个vp出块
                    if(fork.miners[i+3])
                    {
                        p_b[i+3] = c_b[1];
                        p_b[i+nv+3] = c_b[1];
                    }
                    //不是这个vp出块
                    else
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                        {
                            p_b[i+3] = c_b[0];
                            p_b[i+nv+3] = c_b[0];
                        }
                        else
                        {
                            p_b[i+3] = c_b[1];
                            p_b[i+nv+3] = c_b[1];
                        }
                    }
                }
                break;
            }
            //vp op
            case 6:{
                //mp
                slt = RandNum(2);
                if(slt == 1)
                    p_b[0] = c_b[0];
                else
                    p_b[0] = c_b[1];
                //op1 op2
                for(i = 1; i < 3; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[i] = c_b[0];
                    else
                        p_b[i] = c_b[1];
                }
                //vp 1-n
                for(i = 0; i < nv; i ++)
                {
                    //是这个vp出块
                    if(fork.miners[i+3])
                    {
                        p_b[i+3] = c_b[1];
                        p_b[i+nv+3] = c_b[1];
                    }
                    //不是这个vp出块
                    else
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                        {
                            p_b[i+3] = c_b[0];
                            p_b[i+nv+3] = c_b[0];
                        }
                        else
                        {
                            p_b[i+3] = c_b[1];
                            p_b[i+nv+3] = c_b[1];
                        }
                    }
                }
                break;
            }
            //im op
            case 10:{
                //mp
                slt = RandNum(2);
                if(slt == 1)
                    p_b[0] = c_b[0];
                else
                    p_b[0] = c_b[1];
                //op1 op2
                for(i = 1; i < 3; i ++)
                {
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[i] = c_b[0];
                    else
                        p_b[i] = c_b[1];
                }
                //vp 1-n
                for(i = 0; i < nv; i ++)
                {
                    //是这个im出块
                    if(fork.miners[i+nv+3])
                    {
                        p_b[i+nv+3] = c_b[1];
                        p_b[i+3] = c_b[1];
                    }
                    //不是这个im出块
                    else
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                        {
                            p_b[i+3] = c_b[0];
                            p_b[i+nv+3] = c_b[0];
                        }
                        else
                        {
                            p_b[i+3] = c_b[1];
                            p_b[i+nv+3] = c_b[1];
                        }
                    }
                }
                break;
            }
            //i2 v2 o2
            case -1:{
                switch(fork.type2)
                {
                //o2
                case 1:{
                    int cnt = 0;
                    Block* slt_b[nv]={NULL,};
                    for(i = 2; i < nv+2; i ++)
                    {
                        if(c_b[i] != NULL)
                        {
                            c_b[i]->ishold = 0;
                            p_b[i+1] = c_b[i];
                            p_b[i+1+nv] = c_b[i];
                            slt_b[cnt] = c_b[i];
                            cnt ++;
                        }
                    }
                    //mp
                    slt = RandNum(cnt+2);
                    if(slt == cnt+2)
                        p_b[0] = c_b[0];
                    else if(slt == cnt+1)
                        p_b[0] = c_b[1];
                    else
                        p_b[0] = slt_b[slt-1];
                    //vp
                    for(i = 2; i < nv+2; i ++)
                    {
                        if(c_b[i] == NULL)
                        {
                            slt = RandNum(cnt+2);
                            if(slt == cnt+2)
                            {
                                p_b[i+1] = c_b[0];
                                p_b[i+1+nv] = c_b[0];
                            }
                            else if(slt == cnt+1)
                            {
                                p_b[i+1] = c_b[1];
                                p_b[i+1+nv] = c_b[1];
                            }
                            else
                            {
                                p_b[i+1] = slt_b[slt-1];
                                p_b[i+1+nv] = slt_b[slt-1];
                            }
                        }
                    }
                    //op
                    for(i = 1; i < 3; i ++)
                    {
                        slt = RandNum(cnt+2);
                        if(slt == cnt+2)
                            p_b[i] = c_b[0];
                        else if(slt == cnt+1)
                            p_b[i] = c_b[1];
                        else
                            p_b[i] = slt_b[slt-1];
                    }
                    break;
                }
                //v2
                case 2:{
                    //mp
                    slt = RandNum(2);
                    if(slt == 1)
                        p_b[0] = c_b[0];
                    else
                        p_b[0] = c_b[1];
                    //op1 op2
                    for(i = 1; i < 3; i ++)
                    {
                        slt = RandNum(2);
                        if(slt == 1)
                            p_b[i] = c_b[0];
                        else
                            p_b[i] = c_b[1];
                    }
                    //vp 1-n
                    int tmp = 0;
                    for(i = 0; i < nv; i ++)
                    {
                        //是这个vp出块
                        if(fork.miners[i+3])
                        {
                            p_b[i+3] = c_b[tmp];
                            p_b[i+nv+3] = c_b[tmp];
                            tmp++;
                        }
                        //不是这个vp出块
                        else
                        {
                            slt = RandNum(2);
                            if(slt == 1)
                            {
                                p_b[i+3] = c_b[0];
                                p_b[i+nv+3] = c_b[0];
                            }
                            else
                            {
                                p_b[i+3] = c_b[1];
                                p_b[i+nv+3] = c_b[1];
                            }
                        }
                    }
                    break;
                }
                //i2
                case 4:{
                    c_b[0] = NULL; c_b[1] = NULL;
                    break;
                }
                default:
                    break;
                }
                break;
            }
            default:
                printf("P ERROR2!\n");
                return Gen_b;
            }
            if(fork.type0 != 4 && fork.type2 != 4)
            {
                for(i = 0; i < nv+2; i ++)
                    c_b[i] = NULL;
                hold = 0;
                levb ++;
            }
        }
	    //Sleep(1);
	}
	printf("\nblock num is %d\nR_stale is %lf\n",numb, (double)(numb - levb + 1)/numb);

	return p_b[0];
}

/**计算出块奖励 标记主链
 * 输入尾块
 **/
void Count_Static_Reward(Block* rear, double reward[][3]){
	Block* p;
	p = rear;
	while(p->prev != NULL){
        //printf("%d,%d,%c\n",p->block_num,p->block_level,p->miner);
		if(strcmp(p->miner,"mp") == 0){
			//count_s_block ++;
			reward[0][0] = reward[0][0] + 1.0;
        }
		else if(strcmp(p->miner,"op") == 0){
			//count_h_block ++;
			reward[1][0] = reward[1][0] + 1.0;
		}
        else{
            int h = GetVPnum(p->miner);
            reward[h+1][0] += 1;
        }
        //mark the main chain
		p->if_mainchain = 1;
		p = p->prev;
	}
}

/**计算叔侄奖励
 * 输入目标块
 **/
void Reference_uncle_block(Block* target_b, double reward[][3]){
	int max_distance = 7, max_refer_num = 2;
	int refer_num = 0, refer_distance = 1, isref = 0;
	Block *p,*re;

	p = target_b;
	if(p->prev != NULL)
		p = p->prev;
	while(refer_distance < max_distance && refer_num <= max_refer_num && p->prev != NULL){
		p = p->prev;
		refer_distance ++;
		if(p->next[0] != NULL && p->next[1] != NULL){
		    int i = 0;
            while(p->next[i] != NULL && i < nv+2)
            {
                if(p->next[i]->if_mainchain == 0 && p->next[i]->be_referenced_flag == 0 && p->next[i]->ishold == 0)
                {
                    if(strcmp(p->next[i]->miner,"mp") == 0)
                        reward[0][1] += (8 - refer_distance + 1)*1.0 / 8.0;
                    else if(strcmp(p->next[i]->miner,"op") == 0)
                        reward[1][1] += (8 - refer_distance + 1)*1.0 / 8.0;
                    else
                    {
                        int index = GetVPnum(p->next[i]->miner);
                        reward[index+1][1] += (8 - refer_distance + 1)*1.0 / 8.0;
                    }

                    if(strcmp(target_b->miner,"mp") == 0)
                        reward[0][2] += 1.0/32.0;
                    else if(strcmp(target_b->miner,"op") == 0)
                        reward[1][2] += 1.0/32.0;
                    else
                    {
                        int index = GetVPnum(target_b->miner);
                        reward[index+1][2] += 1.0/32.0;
                    }
                    p->next[i]->be_referenced_flag = 1;
                    isref = 1;
                    re = p->next[i];
                    refer_num ++;
                }
                if(refer_num >= 2)
                    break;
                i ++;
            }
//			if(refer_distance > 2 && isref == 1)
//            {
//                printf("uncle len:%d\t",refer_distance);
//                printf("%d,%d,%s\t",target_b->block_num,target_b->block_level,target_b->miner);
//                printf("%d,%d,%s\n",re->block_num,re->block_level,re->miner);
//            }
            isref = 0;
		}
	}
    target_b->reference_num = refer_num;
}

int main(){
    for(typ=0;typ<2;typ++){
    int times = 0;
    while(times++ < 5){
    double a, t = 0.0 , t_max = 0.99;
    a = typ==BTC ? 0.2 : 0.25;
    theta = typ == BTC ? 0.0005 : 0.025;

	t = typ == BTC ? t_min_BTC : t_min_ETH;
	t_max = typ == BTC ? t_max_BTC : t_max_ETH;

    while(t <= t_max){
        FILE *fp;
        char fpname[50];
        BlockList Gen_b, rear, target_b;
        //mp op vp1-n
        double reward[nv+2][3] = {0.0,};

//        double a, t[] = {0.21}, v[] = {0.2};
        InitParam_equaltau(a, t);
//        InitParam(a,t,v);
        ShowParam();
        printf("\nRound:%d tau:%.3lf\n", times, t);

        Gen_b = BlockListCreate();
        rear = Simulation(Gen_b);
        printf("rear level: %d num:%d\n",rear->block_level,rear->block_num);
        Count_Static_Reward(rear,reward);
        if(MAX_LEVEL < 100)
            Display_BlockChain(Gen_b);

        //计算叔块奖励
        target_b = Gen_b;
        while(target_b->next[0] != NULL ){
            int havemain = 0, i = 0;
            // choose the main chain.
            for(i = 0; i < nv+2; i ++)
            {
                if(target_b->next[i]->if_mainchain == 1)
                {
                    target_b = target_b->next[i];
                    havemain = 1;
                    break;
                }
            }
            if(!havemain){
                printf("chain break!\n");
                Display_BlockChain(target_b->prev->prev);
                break;
            }
            Reference_uncle_block(target_b, reward);
            //printf("%d\n", target_b->block_level);
        }

        //sprintf(fpname,"FAWSim-the=%.3lf-lev=%d-a=%.2lf-v=%.2lf.txt",theta,max_block_level,malicious,victim);
        sprintf(fpname,"%d-n=%d-lev=%d.txt",typ,nv,MAX_LEVEL);
        fp = fopen(fpname,"a+");

        //show rewards
        char ch[3][10] = {"static","uncle","nephew"};
        printf("%10s %10s %10s\n", ch[0], ch[1], ch[2]);
        printf("mp  ");
        for(int j = 0; j < 3; j ++)
            printf("%-10.5lf ",reward[0][j]);
        printf("\nop  ");
        for(int j = 0; j < 3; j ++)
            printf("%-10.5lf ",reward[1][j]);
        for(int i = 0; i < nv; i ++)
        {
            char ch[4];
            sprintf(ch, "VP%d",i+1);
            printf("\n%s ",ch);
            for(int j = 0; j < 3; j ++)
                printf("%-10.5lf ", reward[i+2][j]);
        }
        printf("\n");

        //att revenue in BTC
        double btc_a = 0.0, btc_total = 0.0, btc_rra = 0.0;
        for(int i = 0; i < nv+2; i ++)
            btc_total += reward[i][0];
        for(int i = 0; i < nv; i ++)
        {
            btc_a += im[i] / (im[i] + vh[i]) * reward[i+2][0];
        }
        btc_a += reward[0][0];
        btc_rra = btc_a / btc_total;

        //att revenue in ETH
        double eth_a = 0.0, eth_total = 0.0, eth_rra = 0.0;
        double eth_ar = 0.0, eth_au = 0.0, eth_an = 0.0;
        for(int i = 0; i < nv+2; i ++)
            eth_total += reward[i][0] + reward[i][1] + reward[i][2];
        eth_a = reward[0][0] + reward[0][1] + reward[0][2];
        for(int i = 0; i < nv; i ++)
        {
            eth_ar += im[i] / (im[i] + vh[i]) * reward[i+2][0];
            eth_au += im[i] / (im[i] + vh[i]) * reward[i+2][1];
            eth_an += im[i] / (im[i] + vh[i]) * reward[i+2][2];
        }
        eth_a += eth_ar + eth_au + eth_an;
        eth_rra = eth_a / eth_total;

        printf("BTC: %8.4lf\nETH: %8.4lf\n", btc_rra, eth_rra);

        //tps
        double tps = 0.0;
        for(int i = 0; i < nv+2; i ++)
        {
            tps += reward[i][0];
        }
        tps = tps / rear->block_num;
        printf("TPS: %8.4lf\n", tps);

        //写入文件
        fprintf(fp, "%d %-5.3lf ", typ, t);

        if(typ == BTC)
        {
            fprintf(fp, "%-10.6lf %-10.6lf ", tps, btc_rra);
            fprintf(fp, "%-10.6lf", reward[1][0] / btc_total);
            for(int k = 0; k < nv; k ++)
            {
                fprintf(fp, "%-10.6lf ", vh[k] / (im[k] + vh[k]) * reward[k+2][0] / btc_total);
            }
        }
        //tps rra ra ua na ro uo no rv uv nv
        else
        {
            int bnum = rear->block_num;
            //tps rra
            fprintf(fp, "%-10.6lf %-10.6lf ", tps, eth_rra);
            fprintf(fp, "%-10.6lf %-10.6lf %-10.6lf ", reward[0][0] / bnum, reward[0][1] / bnum, reward[0][2] * 32 / bnum);

            double oh;
            oh = reward[1][0] + reward[1][1] + reward[1][2];
            fprintf(fp, "%-10.6lf",  oh / eth_total);
            fprintf(fp, "%-10.6lf %-10.6lf %-10.6lf ", reward[1][0] / bnum, reward[1][1] / bnum, reward[1][2] * 32 / bnum);
            //r u n, a o v1-n

            for(int k = 0; k < nv; k ++)
            {double vr, vu, vn;
                vr = vh[k] / (im[k] + vh[k]) * reward[k+2][0];
                vu = vh[k] / (im[k] + vh[k]) * reward[k+2][1];
                vn = vh[k] / (im[k] + vh[k]) * reward[k+2][2];
                fprintf(fp, "%-10.6lf ", (vr + vu + vn) / eth_total);
                fprintf(fp, "%-10.6lf %-10.6lf %-10.6lf %-8.5lf %-8.5lf ", reward[k+2][0] / bnum, reward[k+2][1] / bnum, reward[k+2][2] * 32 / bnum, vh[k], im[k]);
            }
        }
        fprintf(fp,"\n");
        fclose(fp);
        ReleaseBlock(rear);
		t += t_dlt;
    }
    }
    }
	return 0;
}
