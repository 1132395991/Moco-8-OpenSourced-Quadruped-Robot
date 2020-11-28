
#include "stm32f4xx.h"
/*
Copyright(C) 2018 Golaced Innovations Corporation. All rights reserved.
OLDX_VMC��������˿� designed by Golaced from ���ݴ���

qqȺ:567423074
�Ա����̣�https://shop124436104.taobao.com/?spm=2013.1.1000126.2.iNj4nZ
�ֻ����̵�ַ��http://shop124436104.m.taobao.com
vmc.lib��stm32F4.lib�·�װ��������ģ������ʽ�����˲�̬�㷨�����Ŀ�
*/

#define USE_DEMO 0   			//1->ʹ�ÿ�ԴDEMO����
#define VIR_MODEL 0  			//ʹ�����������ģ�Ͳ��Ե����㷨  ������
#define DEBUG_MODE 0 			//����ģʽ����������3��						������

#define CHECK_USE_SENSOR 0//����ʹ�û����˴��������ݹ����ŵص�
#define USE_LOW_TORQUE 1	//�Ͷ������
#define HIGE_LEG_TRIG 0 	//��̧�ȹ켣
#define SLIP_MODE     1   //����ģʽ

#define GLOBAL_CONTROL 0  //			ȫ����̬����  0->Ŀǰ�Ƚ��ȶ�
#define DOUBLE_LOOP 1			//			������̬����
#define USE_MODEL_FOR_ATT_CAL 1//	ʹ�û�еģ���ں�IMU������̬��
#define USE_ESO_OBSEVER   1//			ʹ��ESO��Ϊ���ٶȹ۲���
#define USE_ATT_POLAR     1//			ʹ�ü�����ϵ�ǶȲ���
#define USE_NEW_INV_MODLE 1//			ʹ�ð����߶ȵĵ����ڿ��λ��ģ��
#define EN_PLAN_USE_JERK  2//     ʹ����СJERK��˹켣�滮  2->ʹ����ʵ�ٶȹ滮
//-------����ѡ��--------
#define MINI_LITTRO_DOG    //Mocomoco  ����
//#define BIG_LITTRO_DOG	 //Big moco  ����

#define MINI_TO_BIG 0    //ֱ��ʹ��С�����˲����Ŵ����������
//-------ϵͳ����--------
#define MIN_SPD_ST 0.0005
#define T_RST 1.5
#define MAX_FSPD 0.6   //����ٶ�����m/s
#define POS_DEAD 0.05  //��������m
#define YAW_POS_MAX 25

extern float MIN_Z,MAX_SPD,MAX_SPD_RAD;
extern float MAX_Z,MIN_X,MAX_X;
extern float gait_test[5];
extern u8 force_dj_off_reset;
typedef struct 
{
	float x;
	float y;
	float z;
	float zz;
}END_POS;

typedef struct 
{
	float fp,kp,ki,kd,max_ki,max_out;
	float fpp,p,i,d,out;
	float exp,err,interge,err_reg;
	float kp_i,k_i,kd_i,max_ki_i,max_out_i;
	float fp_i,p_i,i_i,d_i,out_i;
	float err_i,interge_i,err_reg_i;
	float in_set_force;
}PID;

extern PID h_pid_all,att_pid_all[3],pos_pid[2],pos_pid_all,extend_pid[4],extend_pid_all;
extern PID att_pid_outter_all[2],att_pid_inner_all[2];


typedef struct 
{ 
  float pos_now[3];
	float spd_now[3];
	float acc_now[3];
	float Tsw;//=0.25;
	float leg_dis;//=0.5;
	float leg_h;//=0.25;
	float spd_lift;//=0.8;
	float scale_x;//=1.25;
	float scale_y;//=0.5;
	float scale_z;
	float scale_lift;//=1.5;
	float max_spd;//=leg_dis/Tsw;
	float limit_spd;//= max_spd*4;
  char flag[3];
	float T12;//=Tsw*0.2;%???????
	float T45;//=T12;
	float T23;//=(Tsw-T12*2)/2;
	float T34;//=T23;

	float p1[3],p2[3],p3[3],p4[3],p5[3];
	float v1[3],v2[3],v3[3],v4[3],v5[3];
	float a1[3],a2[3],a3[3],a4[3],a5[3];
	float a12[3],b12[3],g12[3];
	float a23[3],b23[3],g23[3];
	float a34[3],b34[3],g34[3];
	float a45[3],b45[3],g45[3];
}END_PLANNER;

void get_trajecotry_end( float p0, float v0, float a0, float a,float  b, float g, float t ,float *pos,float *acc,float *spd);
void plan_end(float p0,float v0,float a0,float pf,float vf,float af,float Tf,char flag[3],float *a,float *b,float *g,float *cost);


typedef struct 
{
  char id,trig_state,ground_state,invert_knee;
	int invert_knee_epos[2];
	float ground_state_cnt;
	float sita_limit;
	float spd_dj[3];
	float flt_toqrue;
	float gain_torque;
	float delta_h,delta_h_att_off;
	float kp_trig;
	float time_trig;
	float spd_est_cnt;
	float pid[3][5];
	float ground_force[3];
	END_POS tar_epos;
	u16 PWM_OFF[4];					
	u16 PWM_MIN[4],PWM_MAX[4];
	float PWM_OUT[4];				//PWM���
	float PWM_PER_DEGREE[4];//PWM�Ƕ�����
	int sita_flag[4];				//PWM���ӷ���
	float sita[4];
	float st_time_used;
	float sw_time_used;
	float st_protect_timer;
	END_PLANNER end_planner;
}PARAM;

//���Ƚṹ�����
typedef struct 
{
	float l1,l2,l3,l4;
	float r;
	float sita1,sita2,sita;
	END_POS epos,e_pos_base[2],epos_reg,spd,spd_o,spd_o_angle,tar_epos,tar_epos_force,check_epos,tar_spd,st_pos,tar_pos;
	float sita_reg[2];
	float tar_h,h,dh;
	double jacobi[4];
	char ground,ground_s;
	float ground_force[3];
	float force[3],force_cmd[3];
	float torque[3];
	int flag_fb,flag_rl;
	float force_deng[4];
	PARAM param;
}VMC;



typedef struct 
{ //phase
	char ref_leg;
	float timer[2];
	float t_ref;
	float t_d;
	float ti[4];
	float T_all;
	float T_sw;
	float T_st;
	
	float dS[4];
	float S_target[4];
	float S_now[4];
	float S_st[4];
	float S_sw[4];
	float S_sw_ref_reg;
	int ground_flag[4][2];
	float tg;
	float tg_st;
	float w_gait_change;
	float t_gait_change;
	float min_st_rate;
	int gait_mode[2];
	//event
	char state[4];
}GAIT_SHEC;


int gait_trj_planner_init(VMC* in, GAIT_SHEC* gait,char end_sel);
int gait_trj_get(VMC* in, GAIT_SHEC* gait,float time_now);
void end_plan_test(float dt);
//----------------------------------VMCȫ�ֿ������ṹ��--------------------------
typedef struct 
{
  //----------ϵͳ����----------------
  float end_sample_dt;//����ٶ�΢��ʱ��
	float ground_dump;//���׻�������
	float trig_ground_st_rate;//���Ⱥ�ʹ�ܴ���ֹͣ�ı���
	float ground_rst;//�Դ�������
	float angle_set_trig;//���ȴ���ֹͣ�Ƕ�����
	float control_out_of_maxl;//��̬���������������ȳ�����	
	float att_limit_for_w;//����Ȩ�����ĽǶ�����
	float kp_pose_reset[2];//�Ը�λ��������
	float k_mb;
	//------------on board----------
	END_POS tar_pos_on,tar_spd_on;
	float tar_att_on[3];
  //-----------ϵͳ����--------------
	float att_limit[3];
	float ground_force[4][3];
	float encoder_spd[2];
	float cog_off_use[4];
	END_POS tar_spd_use[2],tar_spd_use_rc;
	float line_z[2];//����ʧ��߶�
	float cog_z_curve[3];//ģ��Ԥ��COG����״̬
	float w[10],w_t[10],w_cmd[10],weight[10];
	float max_l,max_to_end;//����ȳ�
	float dt_size[2];//�������ڱ仯���µĲ������ű���
	char jump_state;
	float jump_tar_param[2];//�߶� ����
	float jump_param_use[5];
	float jump_out[3];
	//-------------ģʽ-------------
	char control_mode_all;
	char have_cmd,have_cmd_rc[2],have_cmd_sdk[2][4],en_sdk,soft_start;
	char control_mode,rc_mode[2];//����ģʽ
	char en_hold_on;
	char en_att_tirg;
	char cal_flag[5];//������У׼��־λ
	char smart_control_mode[3];//pos/spd   high  att/rad
}PARAM_ALL;

typedef struct 
{
	//-------------��Ȩ�����--------------
	u8 key_right;		 //��Ȩ���Ƿ���ȷ
	char your_key[3];//�����Ȩ��
	int board_id[3]; //������ID
	float sita_test[5];//[4]->1  ǿ�ƽǶȲ���
	//---------------------------------------
	u8 ground[2][4];
	END_POS tar_pos,tar_spd,tar_spd_rc;
	float tar_att[3],tar_att_rate[3],tar_att_off[3],ground_off[2];
	//------------------------------------------
	float kp_trig[2],kp_deng_gain[2],kp_deng[4],kd_deng[2],deng_all,kp_deng_ctrl[2][4],k_auto_time,kp_g[2],kp_touch,rst_dead,out_range_force;
  float pid[3][7],att_gain_length[2];
	float delta_ht[2],delta_h_att_off,gain_torque;
	float cog_off[5],off_leg_dis[2];
	//
  float l1,l2,l3,l4,W,H,mess,flt_toqrue;
	float gait_time[4];
	float gait_alfa;//0~1
	float delay_time[3],gait_delay_time,stance_time,stance_time_auto;
	//
	float att_trig[4],att_ctrl[4],att_rate_trig[3],att_rate_ctrl[3],att[3],att_rate[3],att_vm[3],att_rate_vm[3],att_vmo[3],acc[4];
	float body_spd[4];
	END_POS pos,pos_n;
	END_POS spd,spd_n;
	END_POS acc_n,acc_b;
	float acc_norm;
	char use_att,att_imu,trig_mode,gait_on,use_ground_sensor,ground_num,leg_power,power_state;
	float end_dis[4];
	u8 err,unmove,hand_hold,fall,fly;
	PARAM_ALL param;
}VMC_ALL;

extern VMC_ALL vmc_all;
extern VMC vmc[4];
extern GAIT_SHEC gait;
void get_license(void);

void VMC_DEMO(float dt);
void VMC_OLDX_VER1(float dt);
void vmc_init(void);
char power_task(float dt);
char power_task_d(float dt);
char estimate_end_state_d(VMC *in,float dt);
char cal_jacobi(VMC *in);
char cal_jacobi_d(VMC *in);
//----------------------------VMC.lib �����˿�ͷ�ļ�-------------------------------
char estimate_end_state(VMC *in,float dt);
float cosd(double in);
float tand(double in);
float sind(float in);
float dead(float x,float zoom);
void DigitalLPF(float in, float* out, float cutoff_freq, float dt);
float att_to_weight(float in, float dead, char mode, float max);


//-------------------------------SDK--------------------------
#define  MAX_CNT_NUM 35
extern int mission_sel_lock;
extern int mission_state;
extern u8 mission_flag;
void smart_control(float dt);
	
#define WAY_POINT_DEAD 0.2 //m
#define MISSION_CHECK_TIME 0.35//s
#define AUTO_HOME_POS_Z 4 //m	
#define LAND_SPD 0.68//m/s
#define MAX_WAYP_Z 100//m
#define WAY_POINT_DEAD1 0.5 //m
#define LAND_CHECK_G 0.25//g
#define LAND_CHECK_TIME 0.86//s
#define YAW_LIMIT_RATE 10//��
#define DRAW_TIME 5//s
//--------------------------------------------------------------
#define COMMON_INT 0
#define COMMON_CNT 1
#define COMMON_CNT1 2
#define COMMON_CNT2 3
#define SPD_MOVE_CNT 6
#define BODY_MOVE_INIT 7
#define BODY_MOVE_CNT 8
#define DRONE_Z_CNT 9
#define EST_INT 10
#define F_YAW_CNT 11
#define F_YAW_INIT 12
#define MISSION_CNT 13
#define PAN_SEARCH_FLAG 14
#define HOME_CNT 15
#define WAY_INIT 16
#define WAY_CNT 17
#define SET_POS_CNT 18
#define C_SEARCH_INT 19
#define D_DOWN_CNT 20
#define D_DOWN_INT 21
#define F_DOWN_CNT 22
#define F_DOWN_INT 23
#define DELAY_CNT 24
#define LAND_CNT 25
#define LAND_CNT1 26
#define LIGHT_DRAW_CNT1 27
#define LIGHT_DRAW_CNT2 28
#define LIGHT_DRAW_CNT3 29
#define C_SEARCH_T_INT 30
#define C_SEARCH_T_INT1 31
//-------------------------------------------------------------	
#define L 0
#define R 1
#define F 0
#define B 1
#define FL 0
#define BL 1
#define FL1 0
#define BL1 1
#define FL2 2
#define BL2 3
#define Xr 0
#define Yr 1
#define Zr 2
#define P 0
#define I 2
#define D 1
#define FB 3
#define IP 4
#define II 5
#define ID 6
#define PITr 0
#define ROLr 1
#define YAWr 2
#define YAWrr 3

#define MODE_SPD 1
#define MODE_POS 2
#define MODE_RAD 3
#define MODE_ATT 4
#define MODE_BODY 5
#define MODE_GLOBAL 6
#define MODE_ATT_YAW_ONLY 7
#define MODE_ATT_PR_ONLY 8
#define MODE_FAST_WAY 1
#define NMODE_FAST_WAY 0

#define RAD_TO_DEG 180/PI
#define DEG_TO_RAD PI/180
#define PI 3.14159267

//---------------���ʱ�����-----------------
#define DJ_MG995 9.34
#define DJ_MG956 9.34
#define DJ_MG955 9.34
#define DJ_MG945 5.7
#define DJ_MG355 11.34
#define DJ_6221MG 12.64
#define DJ_DSERVO 11.34
#define DJ_9G 9.34
#define DJ_DSB 11.3
#define DJ_KPOWER 11.3
#define DJ_BLS892MG 5.37
#define DJ_EMAX_ES08MA 9.8
#define DJ_MG90S 12
#define DJ_SG92R 11.3