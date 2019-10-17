.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO

.text
	.global Start
	
Start:
	LDR R0, = GPIO_BASE + (PORT_SIZE*PORT_B) + GPIO_PORT_DIN //address button data input
	LDR R1, = GPIO_BASE + (PORT_SIZE*PORT_E) + GPIO_PORT_DOUTSET //address set led data outpout
	LDR R2, = GPIO_BASE + (PORT_SIZE*PORT_E) + GPIO_PORT_DOUTCLR //address clear led data output


	MOV R3, #1
	LSL R3, R3, #LED_PIN  // choose led pin in port register

	MOV R4, #1
	LSL R4, R4, #BUTTON_PIN // expected button push value

	Loop:
	LDR R5, [R0]
	AND R5, R5, R4
	CMP R5, R4
	BNE Turn_On
	B Turn_Off

	Turn_Off:
	STR R3, [R2] // turn off led
	B Loop // branch to loop

	Turn_On:
	STR R3, [R1] //turn on led
	B Loop // branch to loop

NOP // Behold denne på bunnen av fila

