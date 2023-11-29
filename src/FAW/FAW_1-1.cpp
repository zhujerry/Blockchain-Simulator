/**
 * FAW simulation
 * Zhu-20210806
 * 1 attacker 1 victim
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

//malicious miners, victim pool, honest pool
double malicious = 0.45, victim = 0.2, beta = 1-malicious-victim;
double tau = 0.0, theta = 0.2;
double r1 = 1-2*theta, r2 = theta;

double mp, im, vp, op, beta_a, beta_b;
double iv, io, vo, oo;

int max_block_level = 400000;
int tmpmax=-999;

typedef struct Block_NODE {
	int block_num;
	int block_level;
	char miner;                     // m,i,v,o
	int ishold=0;                     // 0 not hold 1 hold
	int if_mainchain;
	int reference_num;				//-1: no block references it. 1: 1; 2: 2;
	int be_referenced_flag;			// = -1;
	struct Block_NODE *next1, *next2, *next3, *prev;
} Block, *BlockList;

BlockList BlockListInsert(BlockList p_block, int block_num, int block_level, char miner){

	Block *p;
	p = (Block*)malloc(sizeof(Block));
	p->block_num = block_num;
	p->block_level = block_level;
	p->miner = miner;
	p->next1 = NULL;
	p->next2 = NULL;
	p->next3 = NULL;
	p -> prev = p_block;
	p -> reference_num = 0;
	p -> be_referenced_flag = 0;
	p -> if_mainchain = 0;

	if (p_block -> next3 != NULL)
		printf("error, all next pointers are full!\nblocknum:%d,blocklevel:%d,miner:%c\n",block_num,block_level,miner);

	if (p_block -> next1 == NULL){
		p_block -> next1 = p;
	}else if (p_block -> next2 == NULL){
		p_block -> next2 = p;
	}else{
		p_block -> next3 = p;
	}

	return p;
}

BlockList BlockListCreate(){
	Block* head = (Block*) malloc(sizeof(Block));
	head -> block_num = 0;
	head -> block_level = 0;
	head -> miner = 'o';
	head -> next1 = NULL;
	head -> next2 = NULL;
	head -> next3 = NULL;
	head -> prev = NULL;
	head->if_mainchain = 1;
	return head;
}

void Display_BlockChain(BlockList Genesis_Block){
	printf("\ndisplay\n");
	Block *p, *pn;
	p = Genesis_Block;
	printf("%d,%d,%c\n", p->block_num,p->block_level,p->miner);
	while(p->next1 != NULL){
        int havemain = 0;
        if(p->next1->if_mainchain){
            printf("*");
            pn = p->next1;
            havemain = 1;
        }else printf(" ");
		printf("%d,%d,%c\t", p->next1->block_num,p->next1->block_level,p->next1->miner);
		if(p->next2 != NULL){
            if(p->next2->if_mainchain){
                printf("*");
                pn = p->next2;
                havemain = 1;
            }else printf(" ");
            printf("%d,%d,%c\t",p->next2->block_num,p->next2->block_level,p->next2->miner);
		}
		if(p->next3 != NULL){
            if(p->next3->if_mainchain){
                printf("*");
                pn = p->next3;
                havemain = 1;
            }else printf(" ");
            printf("%d,%d,%c\t",p->next3->block_num,p->next3->block_level,p->next3->miner);
		}
		p = pn;
		printf("\n");
		if(!havemain){
            printf("break!\n\n");
            break;
		}
	}
}

//generate a exponential variable
double GenerateExponential(double lambda){
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

int Argmax(int a[], int len){
    int M = -100;
    int index = 0;
    tmpmax = -999;
    for(int i = 0; i < len; i ++)
    {
        if(a[i] > M)
        {
            M = a[i];
            index = i;
        }
    }
    tmpmax = M;
    return index;
}

double GenTau(double a, double v, double c = 0.5){
    double numerator = 0.0, denominator = 1.0, sq = 0.0;
    denominator = a * (v*v*c + v*a*c - 2*v*c - a*c + v + a + c - 1);
    sq = sqrt(-v*v*a*c + v*v*c*c - v*a*a*c + 2*v*a*c*c + a*a*c*c - 2*v*c*c - a*a*c - 2*a*c*c - v*a + 2*v*c + 3*a*c + c*c - a - 2*c + 1);
    numerator = v * (v*c + a*c - sq - a - c + 1);
    return numerator / denominator;
}

//设定速率，计算tau
void InitParam(double a, double v, double c = 0.5){
    double b = 1 - a - v;
    double ba = c * b, bb = (1-c) * b;

    tau = GenTau(a, v);

    mp = a * (1-tau);
    im = a * tau;
    beta_a = ba;
    beta_b = bb;

    double ta = tau * a;
    double normalize = 1-(1-tau)*malicious;

    ta = ta / normalize;
    v = v / normalize;
    ba = ba / normalize;
    bb = bb / normalize;

    iv = ta * v / (1 - ta) + v * ta / (1 - v);
    io = ta * ba / (1 - ta) + ba * ta / (1 - ba) + ta * bb / (1 - ta) + bb * ta / (1 - bb);
    vo = v * ba / (1 - v) + ba * v / (1 - ba) + v * bb / (1 - v) + bb * v / (1 - bb);
    oo = ba * bb / (1 - ba) + bb * ba / (1 - bb);
}

void InitParam_b(double a, double v, double c = 0.5){
    double b = 1 - a - v;
    double ba = c * b, bb = (1-c) * b;

    tau = GenTau(a, v);

    double normalize = 1-(1-tau)*a;
    mp = a * (1-tau);
    im = a * tau;
    beta_a = ba;
    beta_b = bb;

    im = im / normalize;
    vp = v / normalize;
    v = v / normalize;
    op = beta / normalize;
    ba = ba / normalize;
    bb = bb / normalize;

    iv = im * v / (1 - im) + v * im / (1 - v);
    io = im * ba / (1 - im) + ba * im / (1 - ba) + im * bb / (1 - im) + bb * im / (1 - bb);
    vo = v * ba / (1 - v) + ba * v / (1 - ba) + v * bb / (1 - v) + bb * v / (1 - bb);
    oo = ba * bb / (1 - ba) + bb * ba / (1 - bb);

    r1 = 1 - mp - 2*theta;
}

void ShowParam(){
    printf("\n******** System Parameters **********\n");
    printf("\n--- Computational Power ---\n");
    printf("malicious miners: %.2lf\nvictim pool: %.2lf\nother pools: %.2lf\ntau=%.4lf\n", malicious, victim, beta, tau);
    printf("malicious pool: %.2lf\ninfiltration miner: %.2lf\nvictim pool: %.2lf\nother pools: %.2lf\n", mp, im, vp, op);
    printf("\n----  Fork Probability ----\n");
    printf("iv=%-10.4lfio=%-10.4lfvo=%-10.4lfoo=%-10.4lf\n", iv,io,vo,oo);
    printf("\n-------- Settings --------\n");
    printf("r1=%.4lf r2=%.4lf\n",r1,r2);
    printf("Max Level=%d\n",max_block_level);
    printf("\n*************************************\n");
}

BlockList Simulation(BlockList Genesis_Block){
    //下轮出块时间
	double next_event_time, current_time, next_MP_time, next_IM_time, next_VP_time, next_OP_time;
	double next_VO_time, next_OO_time, next_IO_time, next_IV_time;
	//本轮生成区块数量
	int gennum = 1;
	//IM持有数量
	int hold = 0;
	//分叉情况，i v o1 o2，有分叉置1，没分叉置0，值域{0,3,6,10,11}
	int isfork = 0;
	//区块高度
	int block_level_m = 1, block_level_i = 1, block_level_v = 1, block_level_h = 1, block_level = 1;
	int block_num, block_num_f1, block_num_f2;
	//出块矿工，1 mp, 2 im, 3 vp, 4 op;
	int next_event_type = 0;
	//选择时的随机数
	int slt = 0;

	Block *p_mp, *p_im, *p_vp, *p_h1, *p_h2,
	//新单块，分叉块*2，持有块
	*current_block, *current_block_1, *current_block_2, *current_block_3;

	p_mp = Genesis_Block;
	p_im = Genesis_Block;
	p_vp = Genesis_Block;
	p_h1 = Genesis_Block;
	p_h2 = Genesis_Block;

	block_num = 0;
	current_time = 0.0;
    srand(time(NULL));

	while(block_level <= max_block_level){
	    next_event_time = 999;
//	    next_MP_time = 999; next_IM_time = 999; next_VP_time = 999; next_OP_time = 999;
//	    next_VO_time = 999; next_OO_time = 999; next_IO_time = 999; next_IV_time = 999;
	    next_MP_time = GenerateExponential(mp);
        next_IM_time = GenerateExponential(im*r1);
        next_VP_time = GenerateExponential(victim*r1);
        next_OP_time = GenerateExponential(op*r1);
        next_IV_time = GenerateExponential(iv*r2);
        next_IO_time = GenerateExponential(io*r2);
        next_VO_time = GenerateExponential(vo*r2);
        next_OO_time = GenerateExponential(oo*r2);
        //printf("%10.4lf%10.4lf%10.4lf%10.4lf%10.4lf%10.4lf%10.4lf%10.4lf\n",next_MP_time,next_IM_time,next_VP_time,next_OP_time,next_VO_time,next_OO_time,next_IO_time,next_IV_time);

        if (next_MP_time < next_event_time){
            next_event_time = next_MP_time;
            next_event_type = 1;
        }
        if (next_IM_time < next_event_time){
            next_event_time = next_IM_time;
            next_event_type = 2;
        }
        if (next_VP_time < next_event_time){
            next_event_time = next_VP_time;
            next_event_type = 3;
        }
        if (next_OP_time < next_event_time){
            next_event_time = next_OP_time;
            next_event_type = 4;
        }
        if (next_IV_time < next_event_time){
            next_event_time = next_IV_time;
            next_event_type = 23;
        }
        if (next_IO_time < next_event_time){
            next_event_time = next_IO_time;
            next_event_type = 24;
        }
        if (next_VO_time < next_event_time){
            next_event_time = next_VO_time;
            next_event_type = 34;
        }
        if (next_OO_time < next_event_time){
            next_event_time = next_OO_time;
            next_event_type = 44;
        }
        current_time = next_event_time;
        if(next_event_type < 10)
            block_num ++;
        else{
            //printf("fork!\n");
            block_num += 2;
            block_num_f1 = block_num - 1;
            block_num_f2 = block_num;
        }
        if(max_block_level < 1000)
            printf("%d,%d\n",block_level,next_event_type);
        //区块生成
        switch(next_event_type){
        //MP
        case 1:
            switch(isfork){
                //no fork
                case 0:
                    current_block = BlockListInsert(p_mp, block_num, block_level_m, 'm');
                    break;
                //natural fork
                case 3:
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block = BlockListInsert(p_h1, block_num, block_level_m, 'm');
                    else
                        current_block = BlockListInsert(p_h2, block_num, block_level_m, 'm');
                    break;
                //victim honest
                case 6:
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block = BlockListInsert(p_vp, block_num, block_level_m, 'm');
                    else
                        current_block = BlockListInsert(p_h1, block_num, block_level_m, 'm');
                    break;
                //im honest
                case 10:
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block = BlockListInsert(p_vp, block_num, block_level_m, 'm');
                    else
                        current_block = BlockListInsert(p_h1, block_num, block_level_m, 'm');
                    break;
                //im honest2
                case 11:
                    slt = RandNum(3);
                    if(slt == 1)
                        current_block = BlockListInsert(p_vp, block_num, block_level_m, 'm');
                    else if(slt == 2)
                        current_block = BlockListInsert(p_h1, block_num, block_level_m, 'm');
                    else
                        current_block = BlockListInsert(p_h2, block_num, block_level_m, 'm');
                    break;
                default:
                    printf("MP FORK ERROR!\n");
                    return Genesis_Block;
            }
            isfork = 0;
            hold = 0;
            block_level_m ++;
            break;
        //IM
        case 2:
            if(hold){
                block_num --;
                hold = 1;
                break;
            }
            switch(isfork){
                //no fork
                case 0:
                    current_block_3 = BlockListInsert(p_im, block_num, block_level_i, 'i');
                    break;
                //natural fork
                case 3:
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_3 = BlockListInsert(p_h1, block_num, block_level_i, 'i');
                    else
                        current_block_3 = BlockListInsert(p_h2, block_num, block_level_i, 'i');
                    break;
                //victim honest
                case 6:
//                    slt = RandNum(2);
//                    if (slt == 1)
                        current_block_3 = BlockListInsert(p_vp, block_num, block_level_i, 'i');
//                    else
//                        current_block_3 = BlockListInsert(p_h1, block_num, block_level_i, 'i');
                    break;
                //im honest
                case 10:
//                    slt = RandNum(2);
//                    if(slt == 1)
                        current_block_3 = BlockListInsert(p_im, block_num, block_level_i, 'i');
//                    else
//                        current_block_3 = BlockListInsert(p_h1, block_num, block_level_i, 'i');
                    break;
                case 11:
//                    slt = RandNum(3);
//                    if(slt == 1)
                        current_block_3 = BlockListInsert(p_im, block_num, block_level_i, 'i');
//                    else
//                        current_block_3 = BlockListInsert(p_h1, block_num, block_level_i, 'i');
                    break;
                default:
                    printf("IM FORK ERROR!\n");
                    return Genesis_Block;
            }
            block_level_i ++;
            current_block_3->ishold = 1;
            hold = 1;
            break;
        //VP
        case 3:
            switch(isfork){
                //no fork
                case 0:
                    current_block = BlockListInsert(p_vp, block_num, block_level_v, 'v');
                    break;
                //natural fork
                case 3:
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block = BlockListInsert(p_h1, block_num, block_level_v, 'v');
                    else
                        current_block = BlockListInsert(p_h2, block_num, block_level_v, 'v');
                    break;
                //victim honest
                case 6:
//                    slt = RandNum(2);
//                    if(slt == 1)
                        current_block = BlockListInsert(p_vp, block_num, block_level_v, 'v');
//                    else
//                        current_block = BlockListInsert(p_h1, block_num, block_level_v, 'v');
                    break;
                //im honest
                case 10:
//                    slt = RandNum(2);
//                    if(slt == 1)
                        current_block = BlockListInsert(p_vp, block_num, block_level_v, 'v');
//                    else
//                        current_block = BlockListInsert(p_h1, block_num, block_level_v, 'v');
                    break;
                //im honest2
                case 11:
//                    slt = RandNum(3);
//                    if(slt == 1)
                        current_block = BlockListInsert(p_vp, block_num, block_level_v, 'v');
//                    else
//                        current_block = BlockListInsert(p_h1, block_num, block_level_v, 'v');
                    break;
                default:
                    printf("VP FORK ERROR!\n");
                    return Genesis_Block;
            }
            block_level_v ++;
            isfork = 0;
            hold = 0;
            break;
        //OP
        case 4:
            switch(isfork){
            //no fork
            case 0:
                current_block = BlockListInsert(p_h1, block_num, block_level_h, 'h');
                break;
            //natural fork
            case 3:
                slt = RandNum(2);
                if(slt == 1)
                    current_block = BlockListInsert(p_h1, block_num, block_level_h, 'h');
                else
                    current_block = BlockListInsert(p_h2, block_num, block_level_h, 'h');
                break;
            //victim honest
            case 6:
                slt = RandNum(2);
                if(slt == 1)
                    current_block = BlockListInsert(p_vp, block_num, block_level_h, 'h');
                else
                    current_block = BlockListInsert(p_h1, block_num, block_level_h, 'h');
                break;
            //im honest
            case 10:
                slt = RandNum(2);
                if(slt == 1)
                    current_block = BlockListInsert(p_vp, block_num, block_level_h, 'h');
                else
                    current_block = BlockListInsert(p_h1, block_num, block_level_h, 'h');
                break;
            //im honest2
            case 11:
                slt = RandNum(3);
                if(slt == 1)
                    current_block = BlockListInsert(p_vp, block_num, block_level_h, 'h');
                else if(slt == 2)
                    current_block = BlockListInsert(p_h1, block_num, block_level_h, 'h');
                else
                    current_block = BlockListInsert(p_h2, block_num, block_level_h, 'h');
                break;
            default:
                printf("OP FORK ERROR!\n");
                return Genesis_Block;
            }
            if(hold){
                current_block_3->ishold = 0;
                current_block_1 = current_block_3;
                current_block_2 = current_block;
                isfork = 10;
            }
            else
                isfork = 0;
            block_level_h ++;
            hold = 0;
            break;
        //IV
        case 23:
            switch(isfork){
                //no fork
                case 0:
                    current_block = BlockListInsert(p_vp, block_num_f1, block_level_v, 'v');
                    break;
                //natural fork
                case 3:
                    current_block = BlockListInsert(p_h1, block_num_f1, block_level_v, 'v');
                    break;
                //victim honest
                case 6:
//                    slt = RandNum(2);
//                    if(slt == 1)
                        current_block = BlockListInsert(p_vp, block_num_f1, block_level_v, 'v');
//                    else
//                        current_block = BlockListInsert(p_h1, block_num, block_level_v, 'v');
                    break;
                //im honest
                case 10:
//                    slt = RandNum(2);
//                    if(slt == 1)
                        current_block = BlockListInsert(p_vp, block_num_f1, block_level_v, 'v');
//                    else
//                        current_block = BlockListInsert(p_h1, block_num, block_level_v, 'v');
                    break;
                //im honest2
                case 11:
//                    slt = RandNum(3);
//                    if(slt == 1)
                        current_block = BlockListInsert(p_vp, block_num_f1, block_level_v, 'v');
//                    else
//                        current_block = BlockListInsert(p_h1, block_num, block_level_v, 'v');
                    break;
                default:
                    printf("IV FORK ERROR!\n");
                    return Genesis_Block;
            }
            block_level_v ++;
            isfork = 0;
            hold = 0;
            break;
        //IO
        case 24:
            if(hold){
                current_block_1 = current_block_3;
                switch(isfork){
                    //no fork
                    case 0:
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        break;
                    //natural fork
                    case 3:
                        slt = RandNum(2);
                        if(slt == 1)
                            current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        else
                            current_block_2 = BlockListInsert(p_h2, block_num_f2, block_level_h, 'h');
                        break;
                    //victim honest
                    case 6:
                        slt = RandNum(2);
                        if(slt == 1)
                            current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                        else
                            current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        break;
                    //im honest
                    case 10:
                        slt = RandNum(2);
                        if(slt == 1)
                            current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                        else
                            current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        break;
                    //im honest2
                    case 11:
                        slt = RandNum(3);
                        if(slt == 1)
                            current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                        else if(slt == 2)
                            current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        else
                            current_block_2 = BlockListInsert(p_h2, block_num_f2, block_level_h, 'h');
                        break;
                    default:
                        printf("FORK ERROR!\n");
                        return Genesis_Block;
                }
                block_level_i --;
            }else{
                switch(isfork){
                    //no fork
                    case 0:
                        current_block_1 = BlockListInsert(p_im, block_num_f1, block_level_i, 'i');
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        break;
                    //natural fork
                    case 3:
                        slt = RandNum(2);
                        if(slt == 1)
                            current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_i, 'i');
                        else
                            current_block_1 = BlockListInsert(p_h2, block_num_f1, block_level_i, 'i');
                        slt = RandNum(2);
                        if(slt == 1)
                            current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        else
                            current_block_2 = BlockListInsert(p_h2, block_num_f2, block_level_h, 'h');
                        break;
                    //victim honest
                    case 6:
    //                   slt = RandNum(2);
    //                    if(slt == 1)
                            current_block_1 = BlockListInsert(p_vp, block_num_f1, block_level_i, 'i');
    //                    else
    //                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_i, 'i');
                        slt = RandNum(2);
                        if(slt == 1)
                            current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                        else
                            current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        break;
                    //im honest
                    case 10:
    //                    slt = RandNum(2);
    //                    if(slt == 1)
                            current_block_1 = BlockListInsert(p_im, block_num_f1, block_level_i, 'i');
    //                    else
    //                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_i, 'i');
                        slt = RandNum(2);
                        if(slt == 1)
                            current_block_2 = BlockListInsert(p_im, block_num_f2, block_level_h, 'h');
                        else
                            current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        break;
                    //im honest2
                    case 11:
    //                    slt = RandNum(3);
    //                    if(slt == 1)
                            current_block_1 = BlockListInsert(p_im, block_num_f1, block_level_i, 'i');
    //                    else
    //                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_i, 'i');
                        slt = RandNum(3);
                        if(slt == 1)
                            current_block_2 = BlockListInsert(p_im, block_num_f2, block_level_h, 'h');
                        else if(slt == 2)
                            current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                        else
                            current_block_2 = BlockListInsert(p_h2, block_num_f2, block_level_h, 'h');
                        break;
                    default:
                        printf("FORK ERROR!\n");
                        return Genesis_Block;
                }
            }
            block_level_i ++;
            block_level_h ++;
            isfork = 10;
            hold = 0;
            break;
        //VO
        case 34:
            switch(isfork){
                //no fork
                case 0:
                    current_block_1 = BlockListInsert(p_vp, block_num_f1, block_level_v, 'v');
                    current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                    break;
                //natural fork
                case 3:
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_v, 'v');
                    else
                        current_block_1 = BlockListInsert(p_h2, block_num_f1, block_level_v, 'v');
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                    else
                        current_block_2 = BlockListInsert(p_h2, block_num_f2, block_level_h, 'h');
                    break;
                //victim honest
                case 6:
//                    slt = RandNum(2);
//                    if(slt == 1)
                        current_block_1 = BlockListInsert(p_vp, block_num_f1, block_level_v, 'v');
//                    else
//                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_v, 'v');
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                    else
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                    break;
                //im honest
                case 10:
//                    slt = RandNum(2);
//                    if(slt == 1)
                        current_block_1 = BlockListInsert(p_vp, block_num_f1, block_level_v, 'v');
//                    else
//                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_v, 'v');
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                    else
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                    break;
                //im honest2
                case 11:
//                    slt = RandNum(3);
//                    if(slt == 1)
                        current_block_1 = BlockListInsert(p_vp, block_num_f1, block_level_v, 'v');
//                    else
//                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_v, 'v');
                    slt = RandNum(3);
                    if(slt == 1)
                        current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                    else if(slt == 2)
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                    else
                        current_block_2 = BlockListInsert(p_h2, block_num_f2, block_level_h, 'h');
                    break;
                default:
                    printf("VO FORK ERROR!\n");
                    return Genesis_Block;
            }
            block_level_v ++;
            block_level_h ++;
            isfork = 6;
            hold = 0;
            break;
        //OO
        case 44:
            switch(isfork){
                //no fork
                case 0:
                    current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_h, 'h');
                    current_block_2 = BlockListInsert(p_h2, block_num_f2, block_level_h, 'h');
                    break;
                //natural fork
                case 3:
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_h, 'h');
                    else
                        current_block_1 = BlockListInsert(p_h2, block_num_f1, block_level_h, 'h');
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                    else
                        current_block_2 = BlockListInsert(p_h2, block_num_f2, block_level_h, 'h');
                    break;
                //victim honest
                case 6:
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_1 = BlockListInsert(p_vp, block_num_f1, block_level_h, 'h');
                    else
                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_h, 'h');
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                    else
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                    break;
                //im honest
                case 10:
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_1 = BlockListInsert(p_vp, block_num_f1, block_level_h, 'h');
                    else
                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_h, 'h');
                    slt = RandNum(2);
                    if(slt == 1)
                        current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                    else
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                    break;
                //im honest2
                case 11:
                    slt = RandNum(3);
                    if(slt == 1)
                        current_block_1 = BlockListInsert(p_vp, block_num_f1, block_level_h, 'h');
                    else if(slt == 2)
                        current_block_1 = BlockListInsert(p_h1, block_num_f1, block_level_h, 'h');
                    else
                        current_block_1 = BlockListInsert(p_h2, block_num_f1, block_level_h, 'h');
                    slt = RandNum(3);
                    if(slt == 1)
                        current_block_2 = BlockListInsert(p_vp, block_num_f2, block_level_h, 'h');
                    else if(slt == 2)
                        current_block_2 = BlockListInsert(p_h1, block_num_f2, block_level_h, 'h');
                    else
                        current_block_2 = BlockListInsert(p_h2, block_num_f2, block_level_h, 'h');
                    break;
                default:
                    printf("OO FORK ERROR!\n");
                    return Genesis_Block;
            }
            block_level_h ++;
            if(hold)
            {
                isfork = 11;
                current_block_3->ishold = 0;
            }
            else
                isfork = 3;
            hold = 0;
            break;
        default:
            printf("EVENT ERROR! EVENT TYPE=%d gennum=%d\n",next_event_type,gennum);
            printf("%.4lf  %.4lf  %.4lf  %.4lf\n",next_IV_time,next_IO_time,next_VO_time,next_OO_time);
            return Genesis_Block;
            break;
        }

	    //主链选择
        //无扣块
        if(!hold){
            switch(isfork){
            //no fork, consensus
            case 0:
                p_mp = current_block;
                p_im = current_block;
                p_vp = current_block;
                p_h1 = current_block;
                p_h2 = current_block;
                break;
            //natural fork
            case 3:
                p_h1 = current_block_1;
                p_h2 = current_block_2;
                //mp选择区块
                slt = RandNum(2);
                if(slt == 1)
                    p_mp = current_block_1;
                else
                    p_mp = current_block_2;
                //vp选择区块
                slt = RandNum(2);
                if(slt == 1){
                    p_vp = current_block_1;
                    p_im = current_block_1;
                }
                else{
                    p_vp = current_block_2;
                    p_im = current_block_2;
                }
                break;
            //victim honest
            case 6:
                p_vp = current_block_1;
                p_im = current_block_1;
                //hp选择区块
                slt = RandNum(2);
                if(slt == 1)
                    p_h1 = current_block_1;
                else
                    p_h1 = current_block_2;
                slt = RandNum(2);
                if(slt == 1)
                    p_h2 = current_block_1;
                else
                    p_h2 = current_block_2;
                //mp选择区块
                slt = RandNum(2);
                if(slt == 1)
                    p_mp = current_block_1;
                else
                    p_mp = current_block_2;
                break;
            //im honest
            case 10:
                p_im = current_block_1;
                p_vp = current_block_1;
                //hp选择区块
                slt = RandNum(2);
                if(slt == 1)
                    p_h1 = current_block_1;
                else
                    p_h1 = current_block_2;
                slt = RandNum(2);
                if(slt == 1)
                    p_h2 = current_block_1;
                else
                    p_h2 = current_block_2;
                //mp选择区块
                slt = RandNum(2);
                if(slt == 1)
                    p_mp = current_block_1;
                else
                    p_mp = current_block_2;
                break;
            //im honest2
            case 11:
                p_im = current_block_3;
                p_vp = current_block_3;
                //hp选择区块
                slt = RandNum(3);
                if(slt == 1)
                    p_h1 = current_block_1;
                else if(slt == 2)
                    p_h1 = current_block_2;
                else
                    p_h1 = current_block_3;
                slt = RandNum(3);
                if(slt == 1)
                    p_h2 = current_block_1;
                else if(slt == 2)
                    p_h2 = current_block_2;
                else
                    p_h2 = current_block_3;
                //mp选择区块
                slt = RandNum(3);
                if(slt == 1)
                    p_mp = current_block_1;
                else if(slt == 2)
                    p_mp = current_block_2;
                else
                    p_mp = current_block_3;
                break;
            default:
                printf("P ERROR!\n");
                return Genesis_Block;
            }
            int a[] = {block_level_m,block_level_v,block_level_h};
            int maxidx = Argmax(a,3);
            block_level = tmpmax;
            block_level_m = block_level;
            block_level_i = block_level;
            block_level_v = block_level;
            block_level_h = block_level;
        }
        else{
            //printf("%d, hold!\n",next_event_type);
            p_im = current_block_3;
        }
	    //Sleep(1);
	}

	printf("\nblock num is %d\nR_stale is %lf\n",block_num, (double)(block_num - block_level + 1)/block_num);

	//return the lastest block
	int lst[] = {p_mp->block_level, p_vp->block_level, p_h1->block_level};
	int lstidx = Argmax(lst,3);
	switch(lstidx){
    case 0:
        return p_mp;
    case 1:
        return p_vp;
    case 2:
        return p_h1;
    default:
        printf("LAST BLOCK ERRROR!\n");
        return Genesis_Block;
	}
}

int Count_Static_Reward(BlockList rear, double reward[4][3]){
//	int count_s_block, count_h_block;
//	count_s_block = 0;
//	count_h_block = 0;
	BlockList p;
	p = rear;
	while(p -> prev != NULL ){
        //printf("%d,%d,%c\n",p->block_num,p->block_level,p->miner);
		if(p -> miner == 'm'){
			//count_s_block ++;
			reward[0][0] = reward[0][0] + 1.0;
        }
		else if(p -> miner == 'i'){
			//count_h_block ++;
			reward[1][0] = reward[1][0] + 1.0;
		}
        else if(p -> miner == 'v'){
            reward[2][0] = reward[2][0] + 1.0;
        }
        else if(p -> miner == 'h'){
            reward[3][0] = reward[3][0] + 1.0;
        }
		p -> if_mainchain = 1; 						//mark the main chain
		p = p -> prev;
	}
	return 0;
}

void Reference_uncle_block(BlockList target_block, double reward[4][3]){		//does the target block can reference any blocks?
	int max_distance, max_refer_num;
	int refer_num, refer_distance, isref = 0;
	BlockList p,re;
	max_distance = 6;
	max_refer_num = 2;
	refer_distance = 0;
	refer_num = 0;

	//target_block = target_block -> next1;
	p = target_block;
	if (p-> prev != NULL)
		p = p -> prev;

	while (refer_distance <= max_distance && refer_num <= max_refer_num && p-> prev != NULL){
		p = p -> prev;
		refer_distance ++;
		if(p -> next1 != NULL && p -> next2 != NULL){
			// if next1 is not a main chain and is not be referenced and is found before target block
			if(p->next1 != NULL && p->next1->if_mainchain != 1 && p->next1->be_referenced_flag == 0 && p->next1->ishold == 0 && p->next1->block_num < target_block->block_num && target_block->reference_num < 2){

				if (p -> next1 -> miner == 'm')
					reward[0][1] = reward[0][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
				else if (p -> next1 -> miner == 'i')
					reward[1][1] = reward[1][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
                else if (p -> next1 -> miner == 'v')
					reward[2][1] = reward[2][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
				else if (p -> next1 -> miner == 'h')
					reward[3][1] = reward[3][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
                else ;

				if (target_block -> miner == 'm')
					reward[0][2] = reward[0][2] + 1.0/32.0;
				else if (target_block -> miner == 'i')
					reward[1][2] = reward[1][2] + 1.0/32.0;
                else if (target_block -> miner == 'v')
					reward[2][2] = reward[2][2] + 1.0/32.0;
				else if (target_block -> miner == 'h')
					reward[3][2] = reward[3][2] + 1.0/32.0;
                else ;
				target_block -> reference_num ++;
				refer_num ++;
				p -> next1 -> be_referenced_flag = 1;
				re = p->next1;
				isref = 1;
				//printf("T_b: %d, R_b: %d\n", target_block -> block_num, p -> next1 -> block_num);
			}
			if(p->next2 != NULL && p->next2->if_mainchain != 1 && p->next2->be_referenced_flag == 0 && p->next2->ishold == 0 && p->next2->block_num < target_block->block_num && target_block->reference_num < 2){

				if (p -> next2 -> miner == 'm')
					reward[0][1] = reward[0][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
				else if (p -> next2 -> miner == 'i')
					reward[1][1] = reward[1][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
                else if (p -> next2 -> miner == 'v')
					reward[2][1] = reward[2][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
				else if (p -> next2 -> miner == 'h')
					reward[3][1] = reward[3][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
                else ;

				if (target_block -> miner == 'm')
					reward[0][2] = reward[0][2] + 1.0/32.0;
				else if (target_block -> miner == 'i')
					reward[1][2] = reward[1][2] + 1.0/32.0;
                else if (target_block -> miner == 'v')
					reward[2][2] = reward[2][2] + 1.0/32.0;
				else if (target_block -> miner == 'h')
					reward[3][2] = reward[3][2] + 1.0/32.0;
                else ;
				target_block -> reference_num ++;
				refer_num ++;
				p -> next2 -> be_referenced_flag = 1;
				re = p->next2;
				isref = 1;
				//printf("T_b: %d, R_b: %d\n", target_block -> block_num, p -> next2 -> block_num);
			}
			if(p->next3 != NULL && p->next3->if_mainchain != 1 && p->next3->be_referenced_flag == 0 && p->next3->ishold == 0 && p->next3->block_num < target_block->block_num && target_block->reference_num < 2){
				if (p -> next3 -> miner == 'm')
					reward[0][1] = reward[0][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
				else if (p -> next3 -> miner == 'i')
					reward[1][1] = reward[1][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
                else if (p -> next3 -> miner == 'v')
					reward[2][1] = reward[2][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
				else if (p -> next3 -> miner == 'h')
					reward[3][1] = reward[3][1] + ( 8 - refer_distance)*1.0 / 8.0 ;
                else ;

				if (target_block -> miner == 'm')
					reward[0][2] = reward[0][2] + 1.0/32.0;
				else if (target_block -> miner == 'i')
					reward[1][2] = reward[1][2] + 1.0/32.0;
                else if (target_block -> miner == 'v')
					reward[2][2] = reward[2][2] + 1.0/32.0;
				else if (target_block -> miner == 'h')
					reward[3][2] = reward[3][2] + 1.0/32.0;
                else ;
				target_block -> reference_num ++;
				refer_num ++;
				p -> next3 -> be_referenced_flag = 1;
				re = p->next3;
				isref = 1;
				//printf("T_b: %d, R_b: %d\n", target_block -> block_num, p -> next3 -> block_num);
			}
			if(refer_distance >= 2 && isref == 1)
            {
                printf("uncle len:%d\t",refer_distance);
                printf("%d,%d,%c\t",target_block->block_num,target_block->block_level,target_block->miner);
                printf("%d,%d,%c\n",re->block_num,re->block_level,re->miner);
            }
            isref = 0;
		}
		target_block -> reference_num = refer_num;
	}
}

int main(){
	FILE *fp;
    char fpname[50];
    BlockList Genesis_Block, rear, target_block;
    double reward[4][3] = {0.0,};
    int mflag = 0;

    InitParam_b(malicious,victim);
    ShowParam();

    Genesis_Block = BlockListCreate();
    rear = Simulation(Genesis_Block);
    Count_Static_Reward(rear,reward);
    if(max_block_level < 1000)
        Display_BlockChain(Genesis_Block);

    //计算叔块奖励
    target_block = Genesis_Block;
    while(target_block -> next1 != NULL ){
        int havemain = 0;
		// choose the main chain.
		if(target_block->next1->if_mainchain == 1){
            target_block = target_block->next1;
            havemain = 1;
		}
		else if(target_block->next2->if_mainchain == 1){
            target_block = target_block->next2;
            havemain = 1;
		}
		else if(target_block->next3->if_mainchain == 1){
            target_block = target_block->next3;
            havemain = 1;
		}
		if(!havemain){
            printf("chain break!\n");
            Display_BlockChain(target_block->prev->prev);
            break;
		}
		//if (target_block -> miner == 'h'){
        Reference_uncle_block(target_block, reward);
			//printf("%d, " ,target_block -> block_num);
		//
	}

    sprintf(fpname,"FAWSim-%d-a=%.2lf-v=%.2lf-theta=%.2lf.txt",max_block_level,malicious,victim,theta);
    fp = fopen(fpname,"a+");

//    for(int i = 0; i < 4; i ++)
//        for(int j = 0; j < 3; j ++)
//            reward[i][j] /= rear->block_num;

	char ch[3][10] = {"static","uncle","nephew"};
    for(int i = 0; i < 3; i ++)
    {
        //fprintf(fp,"%-6s\t%-8.4lf%-8.4lf%-8.4lf%-8.4lf\n", ch[i], reward[0][i], reward[1][i], reward[2][i], reward[3][i]);
        printf("%-6s\t%-10.5lf %-10.5lf %-10.5lf %-10.5lf\n", ch[i], reward[0][i], reward[1][i], reward[2][i], reward[3][i]);
    }

    double mr_btc = 0.0, imr_btc = 0.0, rbtc = 0.0;
    double mr_eth = 0.0, imr_eth = 0.0, vpr = 0.0, mpr = 0.0, reth = 0.0;

    //malicious miners BTC reward
    rbtc = reward[0][0] + reward[1][0] + reward[2][0] + reward[3][0];
    imr_btc = malicious*tau/(malicious*tau+victim) * (reward[1][0] + reward[2][0]);
    mr_btc = reward[0][0] + imr_btc;

    //malicious miners ETH reward
    for(int i = 0; i< 3; i ++)
    {
        vpr += reward[1][i] + reward[2][i];
        mpr += reward[0][i];
        reth += reward[0][i] + reward[1][i] + reward[2][i] + reward[3][i];
    }
    imr_eth = malicious*tau/(malicious*tau+victim) * vpr;
    mr_eth = mpr + imr_eth;
    printf("BTC: %8.4lf\nETH: %8.4lf\n", mr_btc/rbtc, mr_eth/reth);
    fprintf(fp, "%-10.6lf%-10.6lf\n", mr_btc/rbtc, mr_eth/reth);
    fclose(fp);

	return 0;
}
