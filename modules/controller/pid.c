 

/*! \file *********************************************************************
 *
 * \brief General PID implementation for AVR.
 *
 * Discrete PID controller implementation. Set up by giving P/I/D terms
 * to Init_PID(), and uses a struct PID_DATA to store internal values.
 *
 *
 *****************************************************************************/

#include "pid.h"
#include "stdint.h"

#define PID_P_SHARE 1
#define PID_I_SHARE 0
#define PID_D_SHARE 0

/*! \brief Initialization of PID controller parameters.
 *
 *  Initialize the variables used by the PID algorithm.
 *
 *  \param p_factor  Proportional term.
 *  \param i_factor  Integral term.
 *  \param d_factor  Derivate term.
 *  \param pid  Struct with PID status.
 */
void pid_Init(int16_t p_factor, int16_t i_factor, int16_t d_factor, struct PID_DATA *pid)
{
	// Start values for PID controller
	pid->sumError         = 0;
	pid->lastProcessValue = 0;
	// Tuning constants for PID loop
	pid->P_Factor = p_factor;
	pid->I_Factor = i_factor;
	pid->D_Factor = d_factor;
	// Limits to avoid overflow
	pid->maxError    = MAX_INT / (pid->P_Factor + 1);
	pid->maxSumError = MAX_I_TERM / (pid->I_Factor + 1);
}

/*! \brief PID control algorithm.
 *
 *  Calculates output from setpoint, process value and PID status.
 *
 *  \param setPoint  Desired value.
 *  \param processValue  Measured value.
 *  \param pid_st  PID status struct.
 */
int16_t pid_Controller_c(int16_t setPoint, int16_t processValue, struct PID_DATA *pid_st)
{
	int16_t errors, p_term, d_term;
	int32_t i_term, ret, temp;

	errors = setPoint - processValue;

	// Calculate Pterm and limit error overflow
	if (errors > pid_st->maxError) {
		p_term = MAX_INT;
	} else if (errors < -pid_st->maxError) {
		p_term = -MAX_INT;
	} else {
		p_term = pid_st->P_Factor * errors;
	}

	// Calculate Iterm and limit integral runaway
	temp = pid_st->sumError + errors;
	if (temp > pid_st->maxSumError) {
		i_term           = MAX_I_TERM;
		pid_st->sumError = pid_st->maxSumError;
	} else if (temp < -pid_st->maxSumError) {
		i_term           = -MAX_I_TERM;
		pid_st->sumError = -pid_st->maxSumError;
	} else {
		pid_st->sumError = temp;
		i_term           = pid_st->I_Factor * pid_st->sumError;
	}

	// Calculate Dterm
	d_term = pid_st->D_Factor * (pid_st->lastProcessValue - processValue);

	pid_st->lastProcessValue = processValue;

	ret = (p_term + i_term + d_term) / SCALING_FACTOR;
	if (ret > MAX_INT) {
		ret = MAX_INT;
	} else if (ret < -MAX_INT) {
		ret = -MAX_INT;
	}

	return ((int16_t)ret);
}

int16_t pid_Controller_asm(int16_t setPoint, int16_t processValue, struct PID_DATA *pid_st){
    
    int8_t pidH,pidL, pid=0;
    char p = PID_P_SHARE +'0';
    
    pid_st->sumError += (setPoint - processValue);
    
    int8_t setPointH = setPoint >> 8;
    int8_t setPointL = setPoint & 128;
    
    int8_t processValueH = processValue >> 8;
    int8_t processValueL = processValue >> 8;
    
    int8_t errorH = (setPoint - processValue) >> 8;
    int8_t errorL = (setPoint - processValue) & 128;
    
    int8_t differenceH = (pid_st->lastProcessValue - processValue) >> 8;
    int8_t differenceL = (pid_st->lastProcessValue - processValue) & 128;
    
    int8_t sumErrorH = (pid_st->sumError) >> 8;
    int8_t sumErrorL = (pid_st->sumError) & 128;
    
    pid_st->lastProcessValue = processValue;
    //overflow/undeflow checks not for P and I controller not neccessery because MAX_INT and MIN_INT are not reachable, maximum error for D share would be 5900 and P Share would be +- 2950 
    
    asm volatile(
        ".equ temp1, 0x10"          "\n\t"
        ".equ temp2, 0x11"          "\n\t"
        ".equ temp3, 0x12"          "\n\t"
        ".equ errorH, 0x13"         "\n\t"
        ".equ errorL, 0x14"         "\n\t"
        "push temp1"                "\n\t"
        "push temp2"                "\n\t"
        "push temp3"                "\n\t"
        
        // calc error
        
        
        
        // P Glied
        
        "mov temp1, %[errorL]"      "\n\t"
        "mulsu temp1, %[pShare]"    "\n\t"
        "mov temp1, R1"             "\n\t"
        "mov pidL, R0"              "\n\t"
        "mov temp2, %[errorH]"      "\n\t"
        "mulsu temp2, %[pShare]"    "\n\t"
        "mov temp2, R1"             "\n\t"
        "add temp1, temp2"          "\n\t"
        "mov pidH, temp1"           "\n\t"
        
        // Overflow Undeflow Check
        
        // calc diff
        
        // D Glied
        
        "mov temp1, %[diffL]"      "\n\t"
        "mulsu temp1, %[dShare]"    "\n\t"
        "mov temp1, R1"             "\n\t"
        "add pidL, R0"              "\n\t"
        "mov temp2, %[diffH]"      "\n\t"
        "mulsu temp2, %[dShare]"    "\n\t"
        "mov temp2, R1"             "\n\t"
        "add temp1, temp2"          "\n\t"
        "add pidH, temp1"           "\n\t"
        
        // Overflow Undeflow Check
        
        // calc sum
        
        // I Glied
        
        //"mov temp1, %[sumL]"      "\n\t"
        //"mulsu temp1, %[iShare]"    "\n\t"
        //"mov temp1, R1"             "\n\t"
        //"add pidL, R0"              "\n\t"
        //"mov temp2, %[sumH]"      "\n\t"
        //"mulsu temp2, %[iShare]"    "\n\t"
        
        // check
        "mov temp2, R1"             "\n\t"
        "add temp1, temp2"          "\n\t"
        "add pidH, temp1"           "\n\t"
        
        //Overflow Undeflow Check
        
        "pop temp3"                 "\n\t"
        "pop temp2"                 "\n\t"
        "pop temp1"                 "\n\t"
        : /* output values */
            [pidH] "=a" (pidH),
            [pidL] "=a" (pidL)
        : /* input values */
            [setValueH] "a" (setPointH),
            [setValueL] "a" (setPointL),
            [errorH] "a" (errorH), 
            [errorL] "a" (errorL),
            [diffH] "a" (differenceH), 
            [diffL] "a" (differenceL),     
            [sumH] "a" (sumErrorH), 
            [sumL] "a" (sumErrorL),  
            [pShare] "M" (PID_P_SHARE),
            [iShare] "M" (PID_I_SHARE),
            [dShare] "M" (PID_D_SHARE)
    );
    
    pid = pidH << 8;
    pid |= pidL;
    
    return pid;
}

/*! \brief Resets the integrator.
 *
 *  Calling this function will reset the integrator in the PID regulator.
 */
void pid_Reset_Integrator(pidData_t *pid_st)
{
	pid_st->sumError = 0;
}
