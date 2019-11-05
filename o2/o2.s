.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO
.include "sys-tick_constants.s" // Register-adresser og konstanter for SysTick

.text
	.global Start
	
Start:

    LDR R0, = SYSTICK_BASE + SYSTICK_CTRL		//systick control and status register setup
    LDR R1, = SysTick_CTRL_TICKINT_Msk
    LDR R2, = SysTick_CTRL_CLKSOURCE_Msk
    ORR R1, R1, R2
    STR R1, [R0]								//toggle ticking and clksource

    LDR R0, = SYSTICK_BASE + SYSTICK_LOAD		//systick reload value register
    LDR R1, = FREQUENCY / 10						//generate 10 interrupts pr second
    STR R1, [R0]
    LDR R2, = SYSTICK_BASE + SYSTICK_VAL
    STR R1, [R2]								//first interrupt will occur after freq/10


	//external interrupt port select
	LDR R0, = GPIO_BASE + GPIO_EXTIPSELH
	MOV R1, 0b1111
	LSL R2, R1, #4 //0b11110000
	MVN R3, R2	  //0b00001111
	LDR R4, [R0]
	AND R5, R3, R4								//reset pin 9
	MOV R6, PORT_B //0b0001
	LSL R7, R6, #4  //0b10000
	ORR R8, R5, R7
	STR R8, [R0]  								//select port b pin 9

	//external interrupt falling edge trigger
	LDR R0, = GPIO_BASE + GPIO_EXTIFALL
	MOV R1, #1
	LSL R1, BUTTON_PIN
	LDR R2,[R0]
	ORR R2, R2, R1
	STR R2,[R0]

	//enable interrupt pin 9
	LDR R0, = GPIO_BASE + GPIO_IEN
	MOV R1, #1
	LSL R1, R1, BUTTON_PIN
	LDR R2, [R0]
	ORR R2, R2, R1
	STR R2, [R0]

	LDR R0, = GPIO_BASE + GPIO_IFC
	MOV R1, #1
	LSL R1, R1, BUTTON_PIN
	STR R1, [R0]								//interrupt flag clear

  	Endless:									//infintate loop
  		B Endless

  	.global SysTick_Handler
	.thumb_func

	SysTick_Handler:
	LDR R0, = tenths
	LDR R1, [R0]
	ADD R1, R1, #1
	CMP R1, #10
	BNE Iterate_tenths
	MOV R1, #0									//reset tenths

	LDR R2, = GPIO_BASE + (PORT_SIZE * LED_PORT) + GPIO_PORT_DOUTTGL
	MOV R3, #1
	LSL R3, R3, LED_PIN
	STR R3, [R2]								//toggle led pin

	LDR R4, = seconds
	LDR R5, [R4]
	ADD R5, R5, #1
	CMP R5, #60
	BNE Iterate_seconds
	MOV R5, #0									//reset seconds

	LDR R6, = minutes
	LDR R7, [R6]
	ADD R7, R7, #1


    Iterate_minutes:
        STR R7, [R6]
    Iterate_seconds:
        STR R5, [R4]
    Iterate_tenths:
        STR R1, [R0]

	BX LR

	.global GPIO_ODD_IRQHandler
	.thumb_func

	GPIO_ODD_IRQHandler:
	LDR R0, = SYSTICK_BASE + SYSTICK_CTRL
    LDR R1, = SysTick_CTRL_ENABLE_Msk
    LDR R2, [R0]
    EOR R2, R1
    STR R2, [R0]								//start clock


	LDR R0, = GPIO_BASE + GPIO_IFC
	MOV R1, #1
	LSL R1, R1, BUTTON_PIN
	STR R1, [R0]								//interrupt flag clear

	BX LR

NOP // Behold denne på bunnen av fila

