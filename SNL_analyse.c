#include<stdio.h>
#include<string.h>
#include<stdlib.h>
//__________________________________________________@全局变量申明
char CODETYPE[20][10]={"ADD","SUB","MULT","DIV","EQC","LTC","READC","WRITEC","RETURNC","ASSIG","AADD","LABEL","JMMP","JUMP0","CALL","VARACT","VALACT","PENTRY","ENDPRO","MENTRY"};
typedef struct token
{
	int lex;
	int line;
	char sym[10];
	struct token *next;
}token;
token *temp,*tokenhead,*tokenlist;
//___________________________________________________@
typedef struct arg
{
	int form; //0.value 1.label  2.addrform
	struct attr
	{
		int value;
		int label;
		struct addr
		{
			char name[10];
			int datalevel;
			int dataoff;
			int kind;
			int access; //0.dir  1.indir
		}addr;
	}attr;
	struct arg *next;
}arg;
arg *argtemp,*arghead,*arglist;
arg *sem[60];//采用数组实现语义栈
int semcur=0,semcurtemp=0,semcurtemp2=0;//语义栈当前指针
int off=7,offtemp=0,intctemp;
int undealpro[30]={0};
int undeal=-1;
int codenum=0;
//____________________________________________________@
/*typedef struct fieldChain
{
	char idname[10];
	int unitType;
	int offset;
	struct fieldChain *next;
}fieldChain;*/
typedef struct typeIR
{
	int size;
	int kind;
	union
	{
		struct
		{
			int low;
			int top;
			int index;
			struct typeIR * elemTy;//指向数组内容
		}ArrayAttr;
		struct SymbTable * body;
	}more;
}typeIR;
typedef struct AttributeIR 
{
	struct typeIR *idtype;	//指向ID内部表示？
	int kind;
	union
	{
		struct
		{
			int access;
			int level;
			int off;
		}VarAttr;
		struct
		{
			int level;
			struct SymbTable *param;
			int code;
			int size;
		}ProcAttr;
	}more;
}AttributeIR;
struct SymbTable
{
	char idname[10];
	AttributeIR attrIR;
	struct SymbTable * next;
}SymbTable;
struct SymbTable *(Scope[10]),*globalp,*tempp,*tempp2,*tempp3,*proctemp;
int level=1,flag=1,inkind=1,dealflag=0;
//__________________________________________________@
typedef struct midcodes
{
	int codekind;
	struct arg *arg1pt;
	struct arg *arg2pt;
	struct arg *arg3pt;
	struct midcodes *former;
	struct midcodes *next;
}midcodes;
struct midcodes *Codeshead,*Codeslist,*Codestemp,*midcodetemp,*tempmidcode;
//_________________________________________________@
//____________________________________产生式处理函数_________________________________*
int (*dealpro[70])();
//_____________________产生式处理函数______________dealpro[18]____类型ID填表_________*______________________________________填表函数
int dealpro18()
{
	globalp->next=malloc(sizeof(SymbTable));
	globalp=globalp->next;
//	strcpy(globalp->idname,"\0");
	strcpy(globalp->idname,tokenlist->sym);
	globalp->attrIR.kind=1/*TYPE整形编码*/;
	globalp->next=NULL;
	globalp->attrIR.idtype=malloc(sizeof(typeIR));
	globalp->attrIR.idtype->kind=inkind;
//	printf("%s\t%d\n",globalp->idname,globalp->attrIR.idtype->kind);
}
//______________________产生式36,52,66处理函数____变量ID填表______dealpro[19]_________*
int dealpro19()
{
	int temp;
//	printf("处理函数19\n");
	temp=findID();
	if(temp==level)
	{
		printf("error:%s\t已定义\n",tokenlist->sym);
		return 0;
	}
//	printf("until now OK!");
	globalp->next=malloc(sizeof(SymbTable));
	globalp=globalp->next;
	strcpy(globalp->idname,tokenlist->sym);
	globalp->attrIR.kind=2/*VAR内部编码*/;
	globalp->next=NULL;
//	globalp->attrIR->VarAttr->access=/*indir,dir内部编码*/;
	globalp->attrIR.more.VarAttr.level=level;
	globalp->attrIR.more.VarAttr.off=off;
	globalp->attrIR.idtype=malloc(sizeof(typeIR));                 //后续处理，可安排在其他函数中
//	printf("inkind=%d\n",inkind);
	globalp->attrIR.idtype->kind=inkind;//先行处理，错了可以该
	if(inkind==1)
		globalp->attrIR.idtype->size=4;
	if(inkind==2)
		globalp->attrIR.idtype->size=1;
	if(inkind==3)
		globalp->attrIR.idtype->size=0;
	if(inkind==4)
		globalp->attrIR.idtype->size=0;
	off=off+globalp->attrIR.idtype->size;
//	printf("%s\t%d\t%d\n",globalp->idname,globalp->attrIR.idtype->kind,globalp->attrIR.more.VarAttr.off);
}
//_____________________产生式44____________过程ID填表函数__dealpro[10]______________*___
int fillprocID()
{
	int temp;
	temp=findID();
	if(temp==level)
	{
		printf("error:%s\t已定义\n",tokenlist->sym);
		return 0;
	}
	globalp->next=malloc(sizeof(SymbTable));
	globalp=globalp->next;
	strcpy(globalp->idname,tokenlist->sym);
	globalp->attrIR.idtype=NULL;/*?*/
	globalp->attrIR.kind=3;/*proc的整形编号*/
//	printf("proc_idname:%s\t\n",globalp->idname);
//	printf("%d\n",globalp->attrIR.kind);
//	globalp->attrIR.more.ProcAttr.param=/*?*/;
//	globalp->attrIR.more.ProcAttr.code=/*目标代码生成阶段填写*/;
//	globalp->attrIR.more.ProcAttr.size=/*?*/;  //->More-> ?
	globalp->next=NULL;
}         

//_____________________dealpro[11]____________________________________*___________________________________________查表函数
int findID()
{
	int templevel;
	flag=1;
	templevel=level;
	while(flag!=0&&templevel>0)
	{
		tempp=Scope[templevel];
		while(tempp!=NULL&&flag!=0)                                        //tempp全局变量
		{
			tempp=tempp->next;
			if(tempp!=NULL)
			{
				flag=strcmp(tempp->idname,tokenlist->sym);                    //相等时返回 0 ?
			}
		}
		if(flag!=0)
			templevel--;

	}
//		printf("templevel=%d\t",templevel);
//	if(templevel>0)
//		printf("IDname:%s\tline=%d\t\n",tempp->idname,tokenlist->line/*tempp->attrIR.idtype->kind*/);
//	printf("调试信息，符号%s在第%d层\t 行数:%d\tinkind=%d\tfalg:%d\n",tempp->idname,templevel,tokenlist->line,tempp->attrIR.idtype->kind,flag);
//	printf("处理函数findID\n");
	return templevel;
}
//______________________________<><44>___________dealpro[20]__[21]_________________________*
int dealpro20()
{
	fillprocID();
	dealpro23();
	proctemp=globalp;
	Scope[++level]=malloc(sizeof(SymbTable));
	globalp=Scope[level];
	off=7;
}
int dealpro21()
{
	globalp->attrIR.more.ProcAttr.param=globalp;
	globalp=Scope[--level];
	while(globalp->next!=NULL)
	{
		globalp=globalp->next;
	}
}
int dealpro23()													//之  Address arg
{
	int temp;
	temp=findID();
	if(temp==0)//错误处理/
	{
		printf("error:ID未定义line:%d\n",tokenlist->line);
		return 0;
	}
	else if(tempp->attrIR.kind!=2&&tempp->attrIR.kind!=3)
	{
			printf("error:ID：%s是type类型,请纠正\n",tempp->idname);
			return 0;
	}

//	printf("%s\t%d\n",tempp->idname,tokenlist->line);
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	strcpy(argtemp->attr.addr.name,tempp->idname);
//	argtemp->attr.addr.datalevel=tempp->attrIR.more.VarAttr.level;
//	printf("我是断点%s\n",tempp->idname);
	if(tempp->attrIR.kind==2)
	{
		argtemp->attr.addr.kind=tempp->attrIR.idtype->kind;
		argtemp->attr.addr.dataoff=tempp->attrIR.more.VarAttr.off;
//		argtemp->attr.addr.dataOff=tempp->attrIR.idtype->access;
	}
	else if(tempp->attrIR.kind==3)
		argtemp->attr.addr.kind=0;//表示是过程ID
	if((semcur+1)>=60)
	{
		printf("error语义栈溢出程序终止\n");
		return 0;
	}
	sem[++semcur]=argtemp;
}
//______________________________________添加中间代码函数_______________________________*______________________________________________________
int makemidcode(int codetype,arg *arg1,arg *arg2,arg *arg3)
{

	midcodetemp=malloc(sizeof(midcodes));
	midcodetemp->codekind=codetype;
	midcodetemp->arg1pt=arg1;
	midcodetemp->arg2pt=arg2;
	midcodetemp->arg3pt=arg3;
	char a1[10]={"\0"},a2[10]={"\0"},a3[10]={"\0"};
	if(midcodetemp->arg1pt!=NULL)
		strcpy(a1,midcodetemp->arg1pt->attr.addr.name);
	if(midcodetemp->arg2pt!=NULL)
		strcpy(a2,midcodetemp->arg2pt->attr.addr.name);
	if(midcodetemp->arg3pt!=NULL)
		strcpy(a3,midcodetemp->arg3pt->attr.addr.name);
	printf("(%s\t,%s\t,%s\t,%s\t)\n",CODETYPE[codetype-1],a1,a2,a3);
	Codeslist->next=midcodetemp;
	tempmidcode=Codeslist;
	Codeslist=midcodetemp;
	Codeslist->former=tempmidcode;
	Codeslist->next=NULL;
	return 1;
}
//_____________________产生式15处理函数____dealpro[15]_________________________________*
int dealpro15()
{
	inkind=1/*integer整形编号*/;
}
//_____________________产生式16处理函数_______dealpro[16]______________________________*
int dealpro16()
{
	inkind=2/*CHAR整形7号*/;
}
//_____________________产生式19处理函数_________dealpro[17]__________________________*
int dealpro17()
{
	inkind=3/*ARRAY整形编号*/;
	tempp3=globalp;
}
//______________________________<105>____赋值语句函数_______dealpro[26]________________*
int dealpro26()
{
	if(sem[semcur]->attr.addr.kind!=sem[semcur-1]->attr.addr.kind)
	{
		printf("参与赋值两变量类型不同，请核实，程序终止\n");
		return 0;
	}
	makemidcode(10/*assign内部编码*/,sem[semcur],sem[semcur-1],NULL);
	semcur=semcur-2;
}
int dealpro40()
{
	globalp->attrIR.idtype->kind=inkind;
}
//___________________________<18>_____________dealpro[41]
int dealpro41()
{
	globalp->attrIR.idtype->kind=4;
	inkind=4;
}
int dealpro42()
{
	int temp;
	temp=findID();
	if(temp==0)
	{
		printf("error:%s未定义\tline:%d\n",tokenlist->sym,tokenlist->line);
		return 0;
	}
	if(tempp->attrIR.kind!=1)
	{
		printf("error:%s不是TYPE型\n",tokenlist->sym);
		return 0;
	}
	inkind=tempp->attrIR.idtype->kind;
}
int dealpro43()		//<22>
{
	tempp2=globalp;
	globalp->attrIR.idtype->more.body=malloc(sizeof(SymbTable));
	globalp=globalp->attrIR.idtype->more.body;
	offtemp=off;
	off=0;
}
int dealpro44()
{
	int a=1;
	struct SymbTable *temp3;
	temp3=tempp2->attrIR.idtype->more.body;
	while(temp3!=globalp)
	{
		a=strcmp(temp3->idname,temp3->next->idname);
		temp3=temp3->next;
	}
	if(a==0)
		printf("error:ID重复定义\n");
	globalp=tempp2;
	off=offtemp+off;
}
int dealpro45()		//VAlue arg
{
		if(semcur>=59)
		{
				printf("语义栈溢出\n");
				return 0;
		}
		argtemp=malloc(sizeof(arg));
		arglist->next=argtemp;
		arglist=arglist->next;
		arglist->next=NULL;
		argtemp->form=1;
		strcpy(argtemp->attr.addr.name,tokenlist->sym);
		char_to_int();
		argtemp->attr.value=intctemp;
		argtemp->attr.addr.kind=1;
		sem[++semcur]=argtemp;
}
int char_to_int()
{
	int t=0,n;
	intctemp=0;
	char itemp[10]={"0123456789"};
	while(tokenlist->sym[t]!='\0')
	{
		for(n=0;n<10;n++)	
		{
			if(tokenlist->sym[t]==itemp[n])
				intctemp=intctemp+n*1;//n* 10的t 次方，回去查查
		}
		t++;
	}
}
int dealpro46()
{
	char_to_int();
	tempp3->attrIR.idtype->more.ArrayAttr.low=intctemp;	
}
int dealpro47()
{
	char_to_int();
	tempp3->attrIR.idtype->more.ArrayAttr.top=intctemp;
	int a;
	typeIR *tempIR;
	tempIR=tempp3->attrIR.idtype->more.ArrayAttr.elemTy;
	for(a=tempp3->attrIR.idtype->more.ArrayAttr.low;a<=tempp3->attrIR.idtype->more.ArrayAttr.top;a++)
	{
		tempIR=malloc(sizeof(typeIR));
		tempIR=tempIR->more.ArrayAttr.elemTy;
		tempIR=NULL;
	}
}
int dealpro48()//<115>
{
//	if((sem[semcur]->value)>tempp->attrIR.idtype->more.ArrayAttr.top||(sem[semcur]->value)<temp->attrIR.ditype->more.ArrayAttr.low)
//	{
//		printf("数组下标越界\n");
//		return 0;
//	}
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	strcpy(argtemp->attr.addr.name,"temp1");
	makemidcode(2,sem[semcur],sem[semcur-1],argtemp);
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	strcpy(argtemp->attr.addr.name,"temp2");
	makemidcode(3,Codeslist/*->former*/->arg3pt,sem[semcur-1],argtemp);
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	strcpy(argtemp->attr.addr.name,"temp3");
	makemidcode(11,sem[semcur-1],Codeslist->/*former->*/arg3pt,argtemp);
	if((semcur)<0)
	{
		printf("error:语义栈为空！\n");
		return 0;
	}
	argtemp->attr.addr.kind=1;
	sem[--semcur]=argtemp;
}
int dealpro49()		//<84>
{
	dealflag=1;
}
int dealpro51()		//+ <101>
{
	undealpro[++undeal]=50;
}
int dealpro50()		//加语句	
{
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	argtemp->attr.addr.kind=1;
	strcpy(argtemp->attr.addr.name,"temp+");
	makemidcode(1,sem[semcur],sem[semcur-1],argtemp);
	sem[--semcur]=argtemp;
}
int dealpro8()		//- <102>
{
	undealpro[++undeal]=9;
}
int dealpro9()		//减语句	
{
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	argtemp->attr.addr.kind=1;
	strcpy(argtemp->attr.addr.name,"temp-");
	makemidcode(2,sem[semcur],sem[semcur-1],argtemp);
	sem[--semcur]=argtemp;
}
int dealpro53()
{
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	argtemp->attr.addr.kind=1;
	strcpy(argtemp->attr.addr.name,"temp<");
	makemidcode(6,sem[semcur-1],sem[semcur],argtemp);
	sem[--semcur]=argtemp;
	argtemp->attr.addr.kind=1;
}
int dealpro52()
{
//	printf("小于语句处理函数已压栈\n");
	dealpro54();
	undealpro[++undeal]=53;
}
int dealpro55()
{
	dealflag=0;
}
int dealpro54()
{
	undealpro[++undeal]=55;
//	printf("表达式开始\n");
}
int dealpro56()
{
	dealpro54();
	undealpro[++undeal]=48;
}
int dealpro12()
{
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	argtemp->attr.addr.kind=1;
	strcpy(argtemp->attr.addr.name,"temp=");
	makemidcode(5,sem[semcur-1],sem[semcur],argtemp);
	sem[--semcur]=argtemp;
	argtemp->attr.addr.kind=1;
}
int dealpro13()
{
	dealpro54();
	undealpro[++undeal]=12;
}
int dealpro6()//*
{
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	argtemp->attr.addr.kind=1;
	strcpy(argtemp->attr.addr.name,"temp*");
	makemidcode(3,sem[semcur-1],sem[semcur],argtemp);
	sem[--semcur]=argtemp;
	argtemp->attr.addr.kind=1;
}
int dealpro7()
{
	undealpro[++undeal]=6;
}
int dealpro4()//除
{
	argtemp=malloc(sizeof(arg));
	arglist->next=argtemp;
	arglist=arglist->next;
	arglist->next=NULL;
	argtemp->form=3;
	argtemp->attr.addr.kind=1;
	strcpy(argtemp->attr.addr.name,"temp/");
	makemidcode(4,sem[semcur-1],sem[semcur],argtemp);
	sem[--semcur]=argtemp;
	argtemp->attr.addr.kind=1;
}
int dealpro5()
{
	undealpro[++undeal]=4;
}
//________________________________<71>_______dealpro[31]_______________________________*
int dealpro31()
{
	argtemp=malloc(sizeof(arg));
	makemidcode(12/*LABEL*/,argtemp,NULL,NULL);
	sem[++semcur]=argtemp;
}        

//_________________________________<109>_____dealpro[32]____________________________*
int dealpro32()
{
	argtemp=malloc(sizeof(arg));
	makemidcode(14/*JUMP0内部表示*/,sem[semcur],argtemp,NULL);
	sem[semcur]=argtemp;
}
//__________________________________<110>_____dealpro[33]________________________*
int dealpro33()
{
	makemidcode(13/*JUMP*/,sem[semcur-1],NULL,NULL);
	makemidcode(12/*LABEL*/,sem[semcur],NULL,NULL);
	semcur=semcur-2;
}
//__________________________________<111>_________dealpro[34]_____________________*_
int dealpro34()
{
	makemidcode(7/*READC*/,sem[semcur],NULL,NULL);
	semcur--;
}
//__________________________________<112>__________dealpro[35]__________________*
int dealpro35()
{
	makemidcode(8/*WRITEC*/,sem[semcur],NULL,NULL);
	semcur--;
}
//______________________过程调用语句____________<79>_<77>___________dealpro[36]_____*
int dealpro36()
{
	int a;
	while(semcurtemp!=0)
	{
	//	if(semcur->/*?*/==/*?*/)
			a=16;
	//	else 	a=17;
		makemidcode(a/*VALACT/VARACT此处还需添加判断语句*/,sem[semcur--],NULL,NULL);
		semcurtemp--;
	}
	makemidcode(15/*CALL*/,sem[semcur],NULL,NULL);
	semcur--;
}
int dealpro37()		//<76>
{
	semcurtemp=semcur;
}
//_______________________________<106>__________dealpro[28]____________________________
int dealpro28()
{
	argtemp=malloc(sizeof(arg));
	strcpy(argtemp->attr.addr.name,"Elarg");
	makemidcode(14/*JUMP0内部编码*/,sem[semcur],argtemp,NULL);
	sem[semcur]=argtemp;
//	printf("调试信息semcur=%d\n",semcur);
}
//_______________________________<107>__________dealpro[29]____________________________*
int dealpro29()
{
	argtemp=malloc(sizeof(arg));
	strcpy(argtemp->attr.addr.name,"OLarg");
	makemidcode(13/*JUMP*/,argtemp,NULL,NULL);
	makemidcode(12/*LABEL内部编码*/,sem[semcur],NULL,NULL);
	sem[semcur]=argtemp;
}
//_______________________________<108>__________dealpro[30]____________________________*
int dealpro30()
{
	makemidcode(12/*LABEL内部表示*/,sem[semcur],NULL,NULL);
	semcur--;
}
//_________________________________过程申明处理函数_____________________________*
//__________________________<55>_________________dealpro[38]_____________________*
int dealpro14()
{
	arg *sizeQ,*levelQ;
	sizeQ=malloc(sizeof(arg));
	strcpy(sizeQ->attr.addr.name,"sizeQ");
	levelQ=malloc(sizeof(arg));
	strcpy(levelQ->attr.addr.name,"levQ");
	makemidcode(18/*PENTRY*/,sem[semcur],sizeQ,levelQ);
	Codestemp=Codeslist;
	semcur--;
}
//__________________________________________<42>____<43>_________________________*
int dealpro22()
{
	makemidcode(19/*ENDPROC*/,NULL,NULL,NULL);
}
int dealpro2()
{
	semcurtemp++;
}
int dealpro3()
{
	dealpro19();
	semcurtemp2++;
}
int proc_to_proc()
{
	dealpro[2]=dealpro2;
	dealpro[3]=dealpro3;
	dealpro[4]=dealpro4;
	dealpro[5]=dealpro5;
	dealpro[6]=dealpro6;
	dealpro[7]=dealpro7;
	dealpro[8]=dealpro8;
	dealpro[9]=dealpro9;
	dealpro[11]=findID;
	dealpro[10]=fillprocID;
	dealpro[12]=dealpro12;
	dealpro[13]=dealpro13;
	dealpro[14]=dealpro14;
	dealpro[15]=dealpro15;
	dealpro[16]=dealpro16;
	dealpro[17]=dealpro17;
	dealpro[18]=dealpro18;
	dealpro[19]=dealpro19;
	dealpro[20]=dealpro20;
	dealpro[21]=dealpro21;
	dealpro[22]=dealpro22;
	dealpro[23]=dealpro23;
	dealpro[26]=dealpro26;
	dealpro[28]=dealpro28;
	dealpro[29]=dealpro29;
	dealpro[30]=dealpro30;
	dealpro[31]=dealpro31;
	dealpro[32]=dealpro32;
	dealpro[33]=dealpro33;
	dealpro[34]=dealpro34;
	dealpro[35]=dealpro35;
	dealpro[36]=dealpro36;
	dealpro[37]=dealpro37;
	dealpro[40]=dealpro40;
	dealpro[41]=dealpro41;
	dealpro[42]=dealpro42;
	dealpro[43]=dealpro43;
	dealpro[44]=dealpro44;
	dealpro[45]=dealpro45;
	dealpro[46]=dealpro46;
	dealpro[47]=dealpro47;
	dealpro[48]=dealpro48;
	dealpro[49]=dealpro49;
	dealpro[50]=dealpro50;
	dealpro[51]=dealpro51;
	dealpro[52]=dealpro52;
	dealpro[53]=dealpro53;
	dealpro[54]=dealpro54;
	dealpro[55]=dealpro55;
	dealpro[56]=dealpro56;
}
///////////////////////////////////////////////////MAIN()///////////////////////////////////////////////////////////////////////////////////$
int main()
{
	semcur=-1;
	int a;
	a=proc_to_proc();
	for(a=0;a<10;a++)
		Scope[a]=NULL;
	Scope[1]=malloc(sizeof(SymbTable));
	globalp=Scope[1];
	Codeshead=malloc(sizeof(midcodes));
	Codeslist=Codeshead;
	tokenhead=malloc(sizeof(struct token));
	tokenlist=tokenhead;
	arghead=malloc(sizeof(struct token));
	arglist=arghead;
	a=lex();
	if(a==0)
	{
		printf("词法错误，程序终止\n");
		return 0;
	}
	a=sym_other();
	if(a==0)
	{
		printf("语法错误，程序终止\n");
		return 0;
	}
	printf("编译成功！\n");
	mfree();
//______________________________________________________________________________________________________________________
	

}
//__________________________________词法分析驱动函数____________________________________________________________________$
int lex()
{
	int T[12][8]={1,7,1,8,9,2,3,0/*0*/,12,12,12,12,12,12,12,12/*1*/,12,12,12,12,12,2,2,12/*2*/,12,12,12,12,12,12,3,12/*3*/,12,12,12,12,12,12,12,12/*4*/,12,12,12,12,12,12,12,12/*5*/,12,12,12,12,12,12,12,12/*6*/,12,6,12,12,12,12,12,12/*7*/,11,11,5,11,11,11,11,11/*8*/,11,11,11,11,11,10,10,11/*9*/,11,11,11,11,4,11,11,11/*10*/,12,12,12,12,12,12,12,12/*11*/};
	char lexy[40][10]={",","+","-","*","/","<","(",")","[","]",";",".","=","ID","INTC","CHARC",":=","..","program","type","integer","char","array","of","record","endxxx","var","procedure","begin","end","if","then","else","while","do","endwh","read","write","return","fi"};
	int line,ch,flag,innum,innum2,to=0,state=0,i;
	char input[77]={",+-*/<()[];.=:'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"};
	char tokentemp[10]="\0";
	char c;
	FILE *lexin;
	FILE *lexout;
	lexin=fopen("SNL","r");
	lexout=fopen("SNLLEXOUT","w");
	line=1;
	while((c=getc(lexin))!=EOF)
	{
		if(c=='\n')
			line++;
		innum=0;
		while(c!=input[innum]&&innum<=76)
			innum++;
		if(innum<=10){ch=0;}
			else if(innum<=14){ch=innum-10;}
			else if(innum<=66){ch=5;}
			else if(innum<=76){ch=6;}
			else ch=7;
			if(T[state][ch]==12)
			{
				switch(state)
				{
					case 1:state=innum2;break;
					case 2:i=17;
					       while(flag!=0&&i<=39)
					       {
						       flag=strcmp(tokentemp,lexy[++i]);// strin
					       }
					       if(i<=39)
						       state=i;
					       else state=13;
					       flag=1;
					       break;
					case 3:state=14;break;
					case 4:state=15;break;
					case 5:state=16;break;
					case 6:state=17;break;
					case 7:state=11;break;
					case 11:return 0;    				//+break？
				}
				tokenlist->lex=state;
				strcpy(tokenlist->sym,"\0");
				strcpy(tokenlist->sym,tokentemp);
				tokenlist->line=line;
				tokenlist->next=malloc(sizeof(token));
				tokenlist=tokenlist->next;
				for(i=0;i<10;i++)
					tokentemp[i]='\0';
				state=0;
				to=0;
			}
			state=T[state][ch];
			if(ch!=7)
				tokentemp[to++]=c;
			innum2=innum;
	}
	tokenlist->next=NULL;
	tokenlist=tokenhead;
	while(tokenlist->next!=NULL)
	{
		fprintf(lexout,"%s  %s  %d  \n",lexy[tokenlist->lex],tokenlist->sym,tokenlist->line);          
		tokenlist=tokenlist->next;
	}
	fclose(lexin);
	fclose(lexout);
	return 1;
}

//_________________________语法，语义，中间代码生成驱动函数___________________________________________________________________________________$_
int sym_other()
{
int chooseproduct[81][40]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,5,5,5,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,11,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,0,0,0,12,12,13,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,17,0,18,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,19,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,23,23,24,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,26,26,26,0,0,0,0,0,0,25,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,27,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,29,0,0,0,0,0,0,0,0,0,28,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,31,30,30,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,33,0,0,0,0,0,0,33,33,33,0,33,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,35,0,0,0,0,0,0,35,35,35,0,35,0,0,34,
34,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,36,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,37,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,40,39,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,43,42,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,44,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,45,0,0,0,0,0,46,0,0,0,0,0,0,46,46,46,0,46,0,46,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,47,0,0,0,0,0,0,47,47,47,0,47,0,47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,48,0,0,49,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,0,0,0,0,0,0,50,50,50,0,50,0,51,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,52,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,53,0,0,53,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,55,0,0,0,0,0,0,55,55,55,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,57,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,58,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,58,0,0,58,0,0,58,58,58,0,59,59,59,59,59,59,59,59,59,59,60,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,0,0,0,0,0,0,0,0,
0,0,0,0,0,66,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,61,0,0,62,0,0,63,64,65,0,0,0,0,0,0,0,68,0,67,0,0,67,0,0,0,0,67,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,69,0,0,69,0,0,0,0,69,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,70,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,71,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,72,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,73,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,74,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,0,0,0,0,0,0,0,76,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,78,77,0,0,0,0,0,78,78,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,
0,0,0,0,0,0,79,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,81,81,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,82,0,0,0,0,0,0,82,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,83,0,0,0,0,0,0,83,83,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,84,85,85,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,84,0,0,0,0,0,0,86,0,0,0,0,0,0,86,86,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,87,87,87,88,88,87,87,87,87,87,87,0,87,0,0,0,0,0,0,0,0,0,0,0,0,87,0,0,0,87,0,87,87,0,87,87,0,0,0,87,0,0,
0,0,0,0,89,0,0,0,0,0,0,91,90,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,92,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,93,93,93,93,93,93,93,93,94,93,93,93,93,93,93,93,93,0,0,0,0,0,0,0,0,93,0,0,0,93,0,93,93,0,93,93,0,0,0,93,0,0,0,0,0,0,0,0,0,0,0,0,0,96,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,97,97,97,97,0,0,97,98,0,0,0,97,0,0,0,97,0,0,0,0,0,0,0,0,97,0,0,0,97,0,97,97,0,97,97,0,0,0,97,0,0,0,0,0,99,0,0,0,0,
0,0,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,101,102,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
103,104,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,
105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,105,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,106,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,107,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,108,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,109,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,110,0,0,0,0,0,0,0,0,0,0,0,111,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,112,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,115,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,118,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120};	
int product[120][11]={87,52,50,1000,1000,1000,1000,1000,1000,1000,0,51,18,1000,1000,1000,1000,1000,1000,1000,1000,0,13,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
75,69,53,1000,1000,1000,1000,1000,1000,1000,0,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,54,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
55,19,1000,1000,1000,1000,1000,1000,1000,1000,0,56,10,58,12,57,1000,1000,1000,1000,1000,0,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,40/*9*/,
55,1000,1000,1000,1000,1000,1000,1000,1000,1000,40/*10*/,13,1000,1000,1000,1000,1000,1000,1000,1000,1000,18/*11*/,59,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
60,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,13,1000,1000,1000,1000,1000,1000,1000,1000,1000,42/*14*/,20,1000,1000,1000,1000,1000,1000,1000,1000,1000,15/*15*/,
21,1000,1000,1000,1000,1000,1000,1000,1000,1000,16/*16*/,61,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,64,1000,1000,1000,1000,1000,1000,1000,1000,1000,41/*18*/,
59,23,9,63,17,62,8,22,1000,1000,17/*19*/,14,1000,1000,1000,1000,1000,1000,1000,1000,1000,46/*20*/,14,1000,1000,1000,1000,1000,1000,1000,1000,1000,47/*21*/,
29,65,24,1000,1000,1000,1000,1000,1000,1000,43/*22*/,66,10,67,59,1000,1000,1000,1000,1000,1000,0,66,10,67,61,1000,1000,1000,1000,1000,1000,0,
1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,44/*25*/,65,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,68,13,1000,1000,1000,1000,1000,1000,1000,1000,19/*27*/,
1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,67,0,1000,1000,1000,1000,1000,1000,1000,1000,0,
1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,70,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
71,26,1000,1000,1000,1000,1000,1000,1000,1000,0,72,10,73,58,1000,1000,1000,1000,1000,1000,0,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
71,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,74,13,1000,1000,1000,1000,1000,1000,1000,1000,19/*36*/,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
73,0,1000,1000,1000,1000,1000,1000,1000,1000,0,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,76,1000,1000,1000,1000,1000,1000,1000,1000,1000,0/*40*/,
77,86,85,10,7,79,6,78,27,1000,0,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,21/*42*/,76,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
13,1000,1000,1000,1000,1000,1000,1000,1000,1000,20/*44*/,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,80,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
81,82,1000,1000,1000,1000,1000,1000,1000,1000,0,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,80,10,1000,1000,1000,1000,1000,1000,1000,1000,0,
83,58,1000,1000,1000,1000,1000,1000,1000,1000,0,83,58,26,1000,1000,1000,1000,1000,1000,1000,0,84,13,1000,1000,1000,1000,1000,1000,1000,1000,3/*52*/,
1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,83,0,1000,1000,1000,1000,1000,1000,1000,1000,0,52,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
87,1000,1000,1000,1000,1000,1000,1000,1000,1000,14/*56*/,29,88,28,1000,1000,1000,1000,1000,1000,1000,0,89,90,1000,1000,1000,1000,1000,1000,1000,1000,0,
1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,22/*59*/,88,10,1000,1000,1000,1000,1000,1000,1000,1000,0,93,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
94,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,95,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,97,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
98,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,91,13,1000,1000,1000,1000,1000,1000,1000,1000,23/*66*/,92,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
99,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,116,104,16,110,1000,1000,1000,1000,1000,1000,0,117,102,30,1000,1000,1000,1000,1000,1000,1000,0,
120,102,33,1000,1000,1000,1000,1000,1000,1000,31/*71*/,122,96,6,36,1000,1000,1000,1000,1000,1000,0,13,1000,1000,1000,1000,1000,1000,1000,1000,1000,23/*73*/,
123,104,6,37,1000,1000,1000,1000,1000,1000,0,38,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,7,100,6,1000,1000,1000,1000,1000,1000,1000,37/*76*/,
1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,36/*77*/,101,104,1000,1000,1000,1000,1000,1000,1000,1000,2,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,36/*79*/,
100,0,1000,1000,1000,1000,1000,1000,1000,1000,0,103,104,1000,1000,1000,1000,1000,1000,1000,1000,0,127,104,113,1000,1000,1000,1000,1000,1000,1000,0/*82*/,
128,105,106,1000,1000,1000,1000,1000,1000,1000,54/*83*/,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0/*84*/,104,114,1000,1000,1000,1000,1000,1000,1000,1000,0,
129,107,108,1000,1000,1000,1000,1000,1000,1000,54,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,106,115,1000,1000,1000,1000,1000,1000,1000,1000,0,
7,104,6,1000,1000,1000,1000,1000,1000,1000,0,14,1000,1000,1000,1000,1000,1000,1000,1000,1000,45/*90*/,109,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
110,13,1000,1000,1000,1000,1000,1000,1000,1000,23/*92*/,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,126,9,104,8,1000,1000,1000,1000,1000,1000,56/*94*/,
111,11,1000,1000,1000,1000,1000,1000,1000,1000,0,112,13,1000,1000,1000,1000,1000,1000,1000,1000,23/*96*/,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
126,9,104,8,1000,1000,1000,1000,1000,1000,0/*98*/,5,1000,1000,1000,1000,1000,1000,1000,1000,1000,52/*99*/,12,1000,1000,1000,1000,1000,1000,1000,1000,1000,13,
1,1000,1000,1000,1000,1000,1000,1000,1000,1000,51/*101*/,2,1000,1000,1000,1000,1000,1000,1000,1000,1000,8/*102*/,3,1000,1000,1000,1000,1000,1000,1000,1000,1000,7,
4,1000,1000,1000,1000,1000,1000,1000,1000,1000,5,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,26,
118,88,31,1000,1000,1000,1000,1000,1000,1000,28/*106*/,119,88,32,1000,1000,1000,1000,1000,1000,1000,29/*107*/,39,1000,1000,1000,1000,1000,1000,1000,1000,1000,30,
121,88,34,1000,1000,1000,1000,1000,1000,1000,32/*109*/,35,1000,1000,1000,1000,1000,1000,1000,1000,1000,33/*110*/,7,1000,1000,1000,1000,1000,1000,1000,1000,1000,34/**/,
7,1000,1000,1000,1000,1000,1000,1000,1000,1000,35/*112*/,124,13,16,110,1000,1000,1000,1000,1000,1000,0,125,15,16,110,1000,1000,1000,1000,1000,1000,0
,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,49/*115*/,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,0,
1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,49/*118*/,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,49/*119*/,1000,1000,1000,1000,1000,1000,1000,1000,1000,
1000,49};
	int state[30],producttemp=0,erroruse=0,i,a;
	int cur=1;
	state[0]=1000;
	state[1]=49;
	tokenlist=tokenhead;
	while(state[cur]!=1000)
	{
		if(tokenlist->next==NULL)
		{
			printf("文件提前结束错误\n");
			return 0;
		}
		erroruse=producttemp;
		producttemp=chooseproduct[state[cur]-49][tokenlist->lex]-1;
//		printf("%d\t%d\t%d\t%d\t%s\n",state[cur],tokenlist->lex,tokenlist->line,producttemp+1,tokenlist->sym);
		cur--;
		if(producttemp==(-1))
		{
			printf("程序第%d\t行\t符号%s\t无法匹配\n",tokenlist->line,tokenlist->sym);
			return 0;
		}
		for(i=0;product[producttemp][i]!=1000;i++)
		{
			state[++cur]=product[producttemp][i];
		}
		a=product[producttemp][10];
		if(a!=0)                      //1000=无处理函数产生式末尾标志
			dealpro[a]();
		int b;
		while(dealflag==1&&undeal>=0)
		{
			dealpro[undealpro[undeal]]();
			undeal--;
		}
		while(state[cur]==tokenlist->lex)
		{
			state[cur--]=1000;
			tokenlist=tokenlist->next;
		}
	}
	return 1;
}
int mfree()
{
	tokenlist=tokenhead;
	while(tokenlist!=NULL)
	{
		temp=tokenlist;
		tokenlist=tokenlist->next;
		free(temp);
	}
	for(semcur=0;semcur<36;semcur++)
	{
		if(sem[semcur]!=NULL)
		{
			free(sem[semcur]);
		}
	}
	arglist=arghead;
	int i=0;
	while(arglist!=NULL)
	{
		argtemp=arglist;
		free(argtemp);
		arglist=arglist->next;
	}
	Codeslist=Codeshead;
	while(Codeslist->next!=NULL)
	{
		Codestemp=Codeslist;
		free(Codestemp);
		Codeslist=Codeslist->next;
	}
	int templevel;
	templevel=1;
	typeIR *tIR,*tIR2;
	while(Scope[templevel]!=NULL)
	{
		tempp=Scope[templevel];
		while(tempp!=NULL)
		{
			if(tempp->attrIR.idtype!=NULL)
			{
				if(tempp->attrIR.idtype->kind==3&&tempp->attrIR.idtype->more.ArrayAttr.elemTy!=NULL)
				{
					tIR=tempp->attrIR.idtype->more.ArrayAttr.elemTy;
					while(tIR!=NULL)
					{
						tIR2=tIR;
						tIR=tIR->more.ArrayAttr.elemTy;
						free(tIR2);
					}
				}
				free(tempp->attrIR.idtype);
			}
			tempp2=tempp;
			free(tempp2);
			tempp=tempp->next;
		}
		templevel++;
	}
	return 1;
}
