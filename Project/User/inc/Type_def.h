
#ifndef		__TYPE_DEF_H
#define		__TYPE_DEF_H

//========================================================================
//                               类型定义
//========================================================================

typedef unsigned char   u8;     //  8 bits 
typedef unsigned int    u16;    // 16 bits 
typedef unsigned long   u32;    // 32 bits 
typedef unsigned char   uchar;     //  8 bits 
typedef signed char     int8;   //  8 bits 
typedef signed int      int16;  // 16 bits 
typedef signed long     int32;  // 32 bits 

typedef unsigned char   uint8;  //  8 bits 
typedef unsigned int    uint16; // 16 bits 
typedef unsigned long   uint32; // 32 bits 

typedef unsigned char   uint8_t;  //  8 bits 
typedef unsigned int    uint16_t; // 16 bits 
typedef unsigned long   uint32_t; // 32 bits

typedef signed char     int8_t;   //  8 bits 
typedef signed int      int16_t;  // 16 bits  
typedef signed long     int32_t; // 32 bits 
typedef unsigned char   u8t;      ///< range: 0 .. 255
typedef signed char     i8t;      ///< range: -128 .. +127
                                      
typedef unsigned short  u16t;     ///< range: 0 .. 65535
typedef signed short    i16t;     ///< range: -32768 .. +32767
                                      
typedef unsigned long   u32t;     ///< range: 0 .. 4'294'967'295
typedef signed long     i32t;     ///< range: -2'147'483'648 .. +2'147'483'647
                                      
typedef float           ft;       ///< range: +-1.18E-38 .. +-3.39E+38
typedef double          dt;       ///< range:            .. +-1.79E+308
typedef unsigned char bool;
typedef enum{
  FALSE     = 0,
  TRUE      = 1
}bt;


/**
 * @brief 日期结构体
 */
typedef struct {

    uint8_t  year;  
    uint8_t  month;  
    uint8_t  day;  	

} Date_Struct;
/**
 * @brief 时间结构体
 */
typedef struct {
    uint8_t  hour;  
    uint8_t  minute;  
    uint8_t  second;  	

} Time_Struct;



//===================================================

#define	TRUE	1
#define	FALSE	0


#define	 true	1
#define	false	0
//===================================================
#ifndef NULL
#define	NULL	0
#endif
//===================================================

#define	Priority_0			0	//中断优先级为 0 级（最低级）
#define	Priority_1			1	//中断优先级为 1 级（较低级）
#define	Priority_2			2	//中断优先级为 2 级（较高级）
#define	Priority_3			3	//中断优先级为 3 级（最高级）

#define ENABLE		1
#define DISABLE		0

#define SUCCESS		0
#define FAIL		-1


#endif
