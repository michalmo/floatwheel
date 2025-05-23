#include "task.h"
#include "math.h"
#include "eeprom.h"
#include "stdlib.h"

#define  BOOT_ANIMATION_COUNT  3
#define  STATUS_BAR_IDLE_MODE_COUNT  2

/**************************************************
+ * Reset LCM Config
+ **************************************************/
static void lcmConfigReset(void)
{
	lcmConfig.isSet = false;
	lcmConfig.debug = false;
	lcmConfig.headlightBrightness = 0;
	lcmConfig.headlightIdleBrightness = 0;
	lcmConfig.statusbarBrightness = 5;
	lcmConfig.boardOff = 0;
	lcmConfig.dutyBeep = 90;
	/*
	lcmConfig.statusBarIdleMode = DEFAULT_IDLE_MODE;
	lcmConfig.chargeCutoffVoltage = 0;
	lcmConfig.bootAnimation = BOOT_DEFAULT;
	lcmConfig.autoShutdownTime = SHUTDOWN_TIME;
	
	EEPROM_ReadByte(BOOT_ANIMATION, &lcmConfig.bootAnimation);
	if (lcmConfig.bootAnimation < 0 || lcmConfig.bootAnimation >= BOOT_ANIMATION_COUNT) {
		lcmConfig.bootAnimation = BOOT_DEFAULT;
	}

	EEPROM_ReadByte(DUTY_BEEP, &lcmConfig.dutyBeep);
	if (lcmConfig.dutyBeep < 1 || lcmConfig.dutyBeep > 100) {
		lcmConfig.dutyBeep = 90;
	}

	EEPROM_ReadByte(STATUS_BAR_IDLE_MODE, &lcmConfig.statusBarIdleMode);
	if (lcmConfig.statusBarIdleMode < 0 || lcmConfig.statusBarIdleMode >= STATUS_BAR_IDLE_MODE_COUNT) {
		lcmConfig.statusBarIdleMode = DEFAULT_IDLE_MODE;
	}

	EEPROM_ReadByte(AUTO_SHUTDOWN, &lcmConfig.autoShutdownTime);
	if (lcmConfig.autoShutdownTime <= 0) {
		lcmConfig.autoShutdownTime = SHUTDOWN_TIME;
	}*/
}

// brightnesses for Gear 1, 2, 3:
int headlight_brightnesses[] = { 0, 150, 255 };
int status_brightnesses[] = { WS2812_1_BRIGHTNESS, WS2812_2_BRIGHTNESS, WS2812_3_BRIGHTNESS };

/**************************************************
 * @brie   :LED_Task()
 * @note   :LEDÈÎÎñ 
 * @param  :ÎÞ
 * @retval :ÎÞ
 **************************************************/
void LED_Task(void)
{
	if(LED_Counter >= 500)
	{
		LED_Counter = 0;
		LED1_FILP;
	}
}

/**************************************************
 * @brie   :KEY1_Task()
 **************************************************/
void KEY1_Task(void)
{
	if(KEY1_State == 0)// || Power_Flag == 3)  //充电器供电按键不起作用
	{
		return;
	}
	
	Idle_Time = 0;
	switch(KEY1_State)
	{
		case 1:         // Click
			if(Power_Flag != 2)
			{
				Power_Flag = 1;  // VESC power on
				lcmConfigReset();
			}
		break;

		case 2:         // Double click
			if(Power_Flag == 2) // Power on completed
			{
				lcmConfig.isSet = false; // Ignore LCM config when manually changing brightness
				Gear_Position++;
				if(Gear_Position == 4)
				{
					Gear_Position = 1;
				}
			}
		break;

		case 3:         // Long press
			if(Power_Flag == 2) // Boot completed
			{
#ifdef ADV2
				Power_Flag = 5;  // external power off
#else
				Power_Flag = 4;  // VESC power off
#endif
				Power_Time = 0;
			}
		break;

		case 4:         // Three presses
			if(Power_Flag == 2) // Boot completed
			{
				if(Buzzer_Flag == 2)
				{
					Buzzer_Flag = 1;
				}
				else
				{
					Buzzer_Flag = 2;
				}
			}
		break;
	}

	KEY1_State = 0;

	// Reset back to showing battery percentage for a couple seconds
}

/**************************************************
 * @brie   :WS2812_Power_Display()
 * @note   :display 1..10 leds depending on power level
 **************************************************/
static void WS2812_Power_Display(uint8_t brightness)
{
	uint8_t numleds = Power_Display_Flag / 10;
	uint8_t remainder = Power_Display_Flag % 10;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	// 20% and below: red
	// 40% and below: yellow
	// > 40% white
	r = brightness;
	if (Power_Display_Flag > 20)
		g = brightness;
	if (Power_Display_Flag > 40)
		b = brightness;

	if (numleds > 0) {
		WS2812_Set_AllColours(1, numleds, r, g, b);
	} else {
		WS2812_Set_AllColours(1, 10, 0, 0, 0);
	}

	if (remainder > 0) {
		r = ((uint16_t)r * remainder) / 10;
		g = g ? r : 0;
		b = b ? r : 0;
		WS2812_Set_Colour(numleds, r, g, b);
	}
}

/**************************************************
 * @brie   : WS2812_VESC()
 * @note   : Display VESC status
 **************************************************/
static void WS2812_VESC(void)
{
	uint8_t i;
	uint8_t pos, red;
	uint8_t green = 0;
	uint8_t blue = WS2812_Measure;
	if (data.rpm > 250) {
		unsigned int fade = 1 + (data.rpm - 250) / 25;	// values from 1 to 31 (max rpm allowed is 1000)
		blue = blue / fade;
	}
	if (data.floatPackageSupported) {
		// make footpad indicators teal if float package commands are received successfully!
		blue = blue / 2;
		green = blue;
	}
	
	switch(WS2812_Flag)
	{
		case 1:// Half Foot Sensors: adc1>2.5V  adc2<2.5V
				WS2812_Set_AllColours(1, 5,0,green, blue);
		break;
		
		case 2:// Half Foot Sensors: adc1<2.5V  adc2>2.5V
				WS2812_Set_AllColours(6, 10,0,green, blue);
		break;
		
		case 3:// Both Foot Sensors: adc1>2.5V  adc2>2.5V
				WS2812_Set_AllColours(1, 10,0,green,blue);
		break;
			
		case 4:// Riding
			
			if (Power_Display_Flag < 30) {
				// Voltage below 30%?
				// Display 1/2 red dots at full brightness above anything else
				WS2812_Power_Display(WS2812_Measure);
			}
			else if (data.dutyCycleNow > 90) {
				WS2812_Set_AllColours(1, NUM_LEDS,255,0,0);
			}
			else if (data.dutyCycleNow > 85) {
				WS2812_Set_AllColours(1, NUM_LEDS-1,255,0,0);
			}
			else if (data.dutyCycleNow > 80) {
				WS2812_Set_AllColours(1, NUM_LEDS-2,WS2812_Measure,WS2812_Measure/2,0);
			}
			else if (data.dutyCycleNow > 70) {
				WS2812_Set_AllColours(1, NUM_LEDS-3,WS2812_Measure/3,WS2812_Measure/3,0);
			}
			else if (Power_Display_Flag < 40) {
				// Voltage below 40%?
				// Display 1/2/3 red dots at full brightness
				WS2812_Power_Display(WS2812_Measure);
			}
			else {
				WS2812_Set_AllColours(1, NUM_LEDS,0,0,0);
			}
		break;

		case 5:
			// Flywheel Mode: just a rando pattern for now
			red = Power_Time % 110;
			green = (Power_Time + 100) % 110;
			blue = (Power_Time - 100) % 110;
			pos = (Power_Time/95) % 10;
			WS2812_Set_Colour(pos,red,green,blue);
		
			// power_time gets capped in hk_it.c at 30k to prevent integer overflow
			// set it down to 10k to let it increment again
			if (Power_Time > 20000)
				Power_Time = 10000;

		break;			
		default:
		break;
	}
}


/**************************************************
 * @brie   :WS2812_Boot()
 * @note   :Boot Animation
 **************************************************/
void WS2812_Boot(void)
{
	uint8_t i;
	uint8_t num = floor(Power_Time / (VESC_BOOT_TIME/10)) + 1;
	uint8_t bootAnims[10][3] = {
		// Default (blue...green)
		//{{10,0,30}, {9,3,27}, {8,6,24}, {7,9,21}, {6,12,18}, {5,15,15}, {4,18,12}, {3,21,9}, {2,24,6}, {1,27,3}},
		{25,0,30}, {22,2,27}, {18,4,24}, {15,6,21}, {12,8,18}, {8,10,15}, {6,12,12}, {4,14,9}, {2,16,6}, {0,18,6},
		// Rainbow
		//{{30,0,0}, {30,15,0}, {30,30,0}, {15,30,0}, {0,30,0}, {0,30,15}, {0,30,30}, {0,15,30}, {0,0,30}, {15,0,30}},
		// Red White Blue
		//{{30,0,0}, {30,30,30}, {0,0,30}, {30,0,0}, {30,30,30}, {0,0,30}, {30,0,0}, {30,30,30}, {0,0,30}, {30,0,0}}
};

	/*if (lcmConfig.bootAnimation < 0 || lcmConfig.bootAnimation >= BOOT_ANIMATION_COUNT) {
		// Invalid boot animation
		lcmConfig.bootAnimation = BOOT_DEFAULT;
	}*/

	while (num > 10) {
		num -= 10;
	}
	for (i=0;i<num;i++) {
		WS2812_Set_Colour(i,bootAnims[i][0],bootAnims[i][1],bootAnims[i][2]);
	}

	for (i = num; i < 10; i++) {
		WS2812_Set_Colour(i,0,0,0);
	}
}

void WS2812_Shutdown(void)
{
	uint8_t brightness = 100;
	int num = 10 - floor(Power_Time / 100);
	if (num < 1) {
		num = 1;
	}
	WS2812_Set_AllColours(num, num, brightness / (11 - num), 0, 0);
}

uint8_t status_brightness = 1;
/**************************************************
 * @brie   : WS2812_Calc_Bri()
 * @note   : Pulsate brightness
 * @param  : times 1 means 200ms
**************************************************/
static uint8_t WS2812_Calc_Bri(uint8_t cnt)
{	
	if(cnt < 50)
	{
		status_brightness++;
	}
	else
	{
		status_brightness--;
	}
	
	if(status_brightness < 1)
	{
		status_brightness = 1;
	}
	
	if(status_brightness > 50)
	{
		status_brightness = 50;
	}
	
	return status_brightness;
}

/**************************************************
 * @brie   :WS2812_Charge()
 * @note   :Power LED display while charging
 **************************************************/
static void WS2812_Charge(void)
{
	static uint8_t cnt = 0;
	WS2812_Power_Display(WS2812_Calc_Bri(cnt));
	cnt++;
	if(cnt == 100)
	{
		cnt = 0;
	}
}	

static void WS2812_Disabled(void)
{
	int brightness = WS2812_Measure;
	if (brightness < 20)
		brightness = 20;

	// 2 red LEDs in the center
	WS2812_Set_AllColours(5, 6, brightness, 0, 0);
}

static void WS2818_Knight_Rider(uint8_t brightness) {
	#define ANIMATION_TICK_TIME 6
	#define TAIL_LENGTH 4
	static uint8_t frame = 0;
	static int8_t direction = 1;
	static int8_t position = 0;

	if (frame >= ANIMATION_TICK_TIME) {
		frame = 0;

		for (uint8_t i = 0; i < NUM_LEDS; i++) {
			int8_t distanceToTail = (direction == 1) ? position - i : i - position;

			// Allow wrap around effect
			if (direction == 1 && i > position) {
				distanceToTail = i + position;
			} else if (direction == -1 && i < position) {
				distanceToTail = (NUM_LEDS - position - 1) + (NUM_LEDS - i - 1);
			}


			uint8_t brightness = (distanceToTail >= 0 && distanceToTail <= TAIL_LENGTH) ?
								WS2812_Measure - distanceToTail * (WS2812_Measure / TAIL_LENGTH) : 0;

			
			WS2812_Set_Colour(i,brightness,0,0);
		}

		// Move the LED position
		position += direction;

		// Change direction when reaching the ends
		if (position == NUM_LEDS - 1  || position == 0) {
			direction *= -1;
		}
	}

	frame++;
}

#define KR_DELAY_MS 10000

// Idle animation:
static void WS2812_Idle()
{
	if (Idle_Time > KR_DELAY_MS) {
		if (Power_Display_Flag < 10 && Idle_Time <= KR_DELAY_MS + 40) {
			// Voltage below 10%? Flash bright red for 40ms!
			WS2812_Set_AllColours(1, 10, 255, 20, 20);
			return;
		}

		if (Power_Display_Flag > 30) {
			// Show KR only above 30% battery
			WS2818_Knight_Rider(WS2812_Measure);
			return;
		}
		
		Idle_Time = 0;
	}
	// Battery mode
	WS2812_Power_Display(WS2812_Measure);
}

static void WS2812_Handtest(void)
{
	static int pulsate = 0;
	int brightness = WS2812_Measure;
	if (brightness < 20)
		brightness = 20;
	pulsate++;
	if (pulsate > 50)
		pulsate = 0;

	// 4 LEDs in the center
	WS2812_Set_AllColours(4, 7, brightness, pulsate, 0);
	if(ADC1_Val > 2.0)
		WS2812_Set_Colour(0, 0, 0, brightness);
	if(ADC2_Val > 2.0)
		WS2812_Set_Colour(9, 0, 0, brightness);
}

/**************************************************
 * @brie   :WS2812_Task()
 * @note   :WS2812任务 
 * @param  :无
 * @retval :无
 **************************************************/
void WS2812_Task(void)
{
	uint8_t i;
	
	if(Power_Flag == 1) {
		Idle_Time = 0;
		WS2812_Boot();
	}
	else if(Charge_Flag == 3) { // Battery fully charged
		WS2812_Set_AllColours(1,10,50,150,50);	// white with a strong green tint
	}
	else if(Charge_Flag == 2) { // Charge display pattern (pulsating led)
		WS2812_Charge();
	}
	else if (WS2812_Display_Flag == 3) {
		WS2812_Shutdown();
	} 
	else if(Power_Flag == 0 || (Power_Flag == 3 && Charge_Flag == 0))	{
		// Board is off
		WS2812_Set_AllColours(1,10,0,0,0);
		WS2812_Display_Flag = 0;
		WS2812_Flag = 0;
		Power_Display_Flag = 0;
	}
	else if (Power_Flag > 2) {
		Idle_Time = 0;
	}
	else {	
		// Power Flag must be 2, aka board is ready or running
		if (lcmConfig.isSet) {
			WS2812_Measure = lcmConfig.statusbarBrightness;
		}
		else if (Gear_Position >= 1 && Gear_Position <= 3)
		{
			WS2812_Measure = status_brightnesses[Gear_Position - 1];
		}

		if (data.state == DISABLED) {
			WS2812_Disabled();
		}
		else if (data.isHandtest) {
			WS2812_Handtest();
		}
		else {
			if (WS2812_Display_Flag == 1) {
				// Idle state - no footpads pressed
				WS2812_Idle();	// Idle animation
			} else {
				Idle_Time = 0;
				WS2812_VESC();
			}
		}
	}

	if (!WS2812_Inhibit)
	{
		// This method disables interrupts, so we only call it when no USART activity is occurring
		WS2812_Refresh();
	}
}

/**************************************************
 * @brie   :Power_Task()
 * @note   :电源任务 
 * @param  :无
 * @retval :无
 **************************************************/
void Power_Task(void)
{
	static uint8_t power_flag_last = 0; //上一次的状态
	static uint8_t power_step = 0;

	if (Power_Flag == 4) {
		if(Power_Time > VESC_SHUTDOWN_TIME)
		{
			Power_Flag = 3;
		}
	}
	else if (Power_Flag == 5) {
		if(Power_Time > VESC_SHUTDOWN_TIME)
		{
			// BMS should shutdown the board by now, otherwise it was a false positive
			Power_Flag = 2;
		}
	}
	
	if(power_flag_last == Power_Flag && Power_Flag != 1)
	{
		return;
	}
	power_flag_last = Power_Flag;
	
	switch(Power_Flag)
	{
		case 1://VESC Power On
			PWR_ON;
			switch(power_step)
			{
				case 0:
					Power_Time = 0;
					power_step = 1;
				break;
				
				case 1:
					if(Power_Time > VESC_BOOT_TIME)
					{
						Power_Flag = 2; // Boot completed
						Gear_Position = 1; // The default setting is 1st gear after power-on.
						Buzzer_Flag = 2;    // The default buzzer sounds when powering on
						power_step = 0;
						WS2812_Display_Flag = 1;
					}
				break;
			}
			
		break;	

		case 3:// VESC is shut down (either auto-shutdown or button press)
			WS2812_Display_Flag = 0;
			PWR_OFF 
		break;

		case 4:// New Power state for shutdown sequence
		case 5:// New Power state for ADV2 external shutdown sequence
			WS2812_Display_Flag = 3;
		break;

		default:
		break;
	}
}

void CheckPowerLevel(float battery_voltage)
{
	#ifdef S50S
	uint16_t battVoltages_mv[11] = {4200, 4075, 4040, 3900, 3820, 3735, 3640, 3520, 3375, 3160, 3000}; //50S
	#endif

	#ifdef P42A
	uint16_t battVoltages_mv[11] = {4200, 4065, 3938, 3854, 3776, 3695, 3618, 3543, 3460, 3342, 3000}; //P42A
	#endif

	#ifdef DG40
	uint16_t battVoltages_mv[11] = {4200, 4047, 3944, 3867, 3799, 3717, 3600, 3498, 3381, 3237, 3000}; //DG40
	#endif

	#ifdef VTC6
	uint16_t battVoltages_mv[11] = {4200, 4064, 4015, 3895, 3821, 3745, 3655, 3559, 3459, 3292, 3000}; // Sony VTC6
	#endif
	
    uint16_t battery_voltage_mv = battery_voltage * 1000;

	if (battery_voltage_mv >= battVoltages_mv[0]) {
		Power_Display_Flag = 100;
	} else if (battery_voltage_mv <= battVoltages_mv[10]) {
		Power_Display_Flag = 0;
	} else {
		for (int i = 1; i <= 10; i++) {
			if (battery_voltage_mv >= battVoltages_mv[i]) {
				Power_Display_Flag = ((100 - i*10) + (10 * (battery_voltage_mv - battVoltages_mv[i])) / (battVoltages_mv[i - 1] - battVoltages_mv[i]));
				break;
			}
		}
	}
}

/**************************************************
 * @brie   :Charge_Task()
 * @note   :Check for charge start/end conditions, only for ADV1
 **************************************************/
#ifdef ADV
void Charge_Task(void)
{
	static uint8_t charge_step = 0;
	bool isAboveCutoff = false;//lcmConfig.chargeCutoffVoltage > 0 && Charge_Voltage > lcmConfig.chargeCutoffVoltage;

	if(Charge_Flag > 0)
	{
		if(V_I == 0 && Charge_Time > 150)
		{
			if((Charge_Current < CHARGE_CURRENT && Charge_Current > 0) || isAboveCutoff) {
				Shutdown_Cnt++;
				if(Shutdown_Cnt>2000)
				{
					if (isAboveCutoff || Charge_Voltage > FULL_VOLTAGE) {
						Charge_Flag = 3;
					}
					else {
						Charge_Flag = 0;
					}
					Charge_Time = 0;
					V_I = 1;
					LED1_ON; // Use ADC3 to measure charge voltage
					CHARGE_OFF;
				}
			}
			else
			{
				Shutdown_Cnt = 0;
			}
		}
		else
		{
			if((Charge_Flag == 2) && (Charge_Time > 150))
			{
				CheckPowerLevel(Charge_Voltage/BATTERY_STRING);
			}
			if((Charge_Flag == 3) && (Shutdown_Cnt > 10))
			{
				if (Charge_Voltage < CHARGING_VOLTAGE)
				{
					// wait for charger to get unplugged to reset back to normal state
					Charge_Flag = 0;
					charge_step = 0;
					Charge_Voltage = 0;
					Charger_Detection_1ms = 0;
				}
			}
		}
	}
	else //	Charge_Flag == 0
	{
		charge_step = 0;
		if(Charge_Voltage > CHARGING_VOLTAGE)// && (Charge_Current > 0.1))
		{
			if(Charger_Detection_1ms > CHARGER_DETECTION_DELAY)
			{
				if (Charge_Flag != 2)
					Charge_Flag = 1;
				WS2812_Display_Flag = 0;
			}
		}
		else {
			Charger_Detection_1ms = 0;
			return;
		}
	}

	switch(charge_step)
	{
		case 0:
			Charge_Time = 0;
			charge_step = 1;
		break;
		
		case 1:
			if(Charge_Time > 1000)  //��ʱ1S
			{
				charge_step = 2;
			}
		break;
		
		case 2:
			CHARGE_ON;
			Charge_Flag = 2;
			charge_step = 3;
			//Power_Flag = 1;	// Boot the VESC
		break;
		
		case 3:
			Charge_Time = 0;
			charge_step = 4;
		break;
			
		case 4:	
			if(Charge_Time > DETECTION_SWITCH_TIME) 
			{
				V_I = 1;
				Charge_Time = 0;
				LED1_ON; // Use ADC3 to measure charge voltage
				charge_step = 5;
			}
		break;
			
		case 5:
			if(Charge_Time > DETECTION_SWITCH_TIME) 
			{
				V_I = 0;
				Charge_Time = 0;
				LED1_OFF; // Use ADC3 to measure charge current
				charge_step = 4;
			}		
		break;
			
		default:
		break;
		
	}
}
#endif

/**************************************************
 * @brief  :Set_Headlights_Brightness()
 * @note   :
 * Brightness can be positive (forward) or negative (backward) - 0..255 (off to max bright)
 * Map to 9999[MIN] ... 0[MAX]
 */
static void Set_Headlights_Brightness(int brightness)
{
	if (brightness > 0) { // FORWARD
		LED_F_OFF;
		LED_B_ON;
	}
	else if (brightness == 0) {
		if (Target_Headlight_Brightness == 0) {
			LED_B_OFF;
			LED_F_OFF;
		}
	}
	else { // BACKWARD
		LED_B_OFF;
		LED_F_ON;
		brightness = -brightness;
	}
	int mapped_brightness = 9999 - brightness * 40;
	if (mapped_brightness < 0)
		mapped_brightness = 0;
	TIM_SetCompare2(TIM1, mapped_brightness);
}

/**************************************************
 * @brief  :Headlights_Task()
 * @note   :Handle direction changes and fading of headlights
 * Smooth fading during transitions is achieved by using positive and 
 * negative brightnesses for forward/backward
 **************************************************/
void Headlights_Task(void)
{
	static uint8_t gear_position_last = 0;
	static bool isForward = false;

	if (Flashlight_Time < 10) {
		return;
	}
	Flashlight_Time = 0;

	if(Power_Flag != 2) // Lights off 
	{
		LED_B_OFF;
		LED_F_OFF;
		TIM_SetCompare2(TIM1,9999);
		Target_Headlight_Brightness = 0;
		Current_Headlight_Brightness = 0;
		return;
	}

	if ((Target_Headlight_Brightness != 0) || (Current_Headlight_Brightness != 0)) {
		if (Current_Headlight_Brightness < Target_Headlight_Brightness) {
			Current_Headlight_Brightness += 3;
			if (Current_Headlight_Brightness > Target_Headlight_Brightness)
				Current_Headlight_Brightness = Target_Headlight_Brightness;
		}
		else if (Current_Headlight_Brightness > Target_Headlight_Brightness) {
			Current_Headlight_Brightness -= 3;
			if (Current_Headlight_Brightness < Target_Headlight_Brightness)
				Current_Headlight_Brightness = Target_Headlight_Brightness;
		}
		Set_Headlights_Brightness(Current_Headlight_Brightness);
	}

	// Set new target
	int new_brightness = Target_Headlight_Brightness;
	if ((data.state < RUNNING_FLYWHEEL) || (ADC1_Val > 2) || (ADC2_Val > 2)) {
		if (lcmConfig.isSet) {
			new_brightness = lcmConfig.headlightBrightness;
		}
		else {
			if (Gear_Position >= 1 && Gear_Position <= 3) {
				new_brightness = headlight_brightnesses[Gear_Position - 1];
			}
		}
		new_brightness *= data.isForward ? 1 : -1;
	}
	else {
		new_brightness = 0;
		if (lcmConfig.isSet) {
			new_brightness = lcmConfig.headlightIdleBrightness;
			float pitch = data.pitch > 0 ? data.pitch : -data.pitch;
			if ((pitch > 75) && (pitch < 105)) {
				// headlights off when the board is upgright (e.g. being carried or leaning against a wall)
				new_brightness = 0;
			}
		}

		if (gear_position_last == Gear_Position && Flashlight_Detection_Time >= 3100) {
			Flashlight_Detection_Time = 3100;
		}
		else {
			// User double-pressed the power button, show the new brightness when idle
			Flashlight_Detection_Time = 0;
			if (Gear_Position >= 1 && Gear_Position <= 3 && !lcmConfig.isSet) {
				new_brightness = headlight_brightnesses[Gear_Position - 1];
			}
			gear_position_last = Gear_Position;
		}
	}
	if (new_brightness != Target_Headlight_Brightness) {
		Target_Headlight_Brightness = new_brightness;
		Flashlight_Time = 0;
	}
}

#ifdef USE_BUZZER

/**************************************************
 * @brie   :Buzzer_Task()
 **************************************************/
void Buzzer_Task(void)
{
	static uint8_t buzzer_step = 0;
	static uint8_t gear_position_last = 0;
	static uint8_t ring_frequency = 0;
	static uint16_t sound_frequency = 0;

	if(Power_Flag < 2 || Buzzer_Flag == 1)
	{
		BUZZER_OFF;
		buzzer_step = 0;
		return;
	}
	else if (Power_Flag == 4 || Power_Flag == 5)
	{
		// Beep when powering off
		if (buzzer_step == 0)
		{
			Buzzer_Ring(50);
			buzzer_step = 2;
		}
		return;
	}
	else if(Buzzer_Frequency == 0 && gear_position_last == Gear_Position)
	{
		BUZZER_OFF;
		buzzer_step = 0;
		return;
	}
	
	if(Buzzer_Frequency != 0)	
	{
		switch(buzzer_step)
		{
			case 0:
				sound_frequency = (uint16_t)(-2.78*Buzzer_Frequency+666);
				Buzzer_Time = 0;
				buzzer_step = 1;
			break;
			
			case 1:
				Buzzer_Ring(sound_frequency>>2);
				buzzer_step = 2;
			break;
			
			case 2:
				if(Buzzer_Time > sound_frequency)
				{	
					buzzer_step = 0;
				}
			break;
				
			default:
			
		  break;
		}
	}
	else
	{
		switch(buzzer_step)
		{
			case 0:
				Buzzer_Time = 0;
				buzzer_step = 1;
			break;
			
			case 1:
				Buzzer_Ring(50);
				buzzer_step = 2;
			break;
			
			case 2:
				if(Buzzer_Time > 400)
				{	
					ring_frequency++;
					buzzer_step = 0;
					if(ring_frequency >= Gear_Position)
					{
						ring_frequency = 0;
						gear_position_last = Gear_Position;
					}
					
				}
			break;
				
			default:
			
		  break;
		}
	}
}

#endif

/**************************************************
 * @brie   :Usart_Task()
 **************************************************/

// Request VESC status every 100 ms
#define USART_INTERVAL_MS 100

// Expect a response from the VESC within 10 ms
// It is important for this to be as small as possble in order to give the WS2812 subsystem an oppoertunity to run. This
// is necessary because WS2812 disables interrupts, so we inhibit it from refreshing while we're waiting for the VESC
// response in order to allow the USART interupts to be handled and not lose any data.
#define USART_TIMEOUT_MS 10

void Usart_Task(void)
{
	static uint8_t usart_step = 0;
	static uint8_t commandIndex = 0; // Store a rotating index so we can implement relevant frequencies of commands
	uint8_t result;

	if(Power_Flag != 2)
	{
		// legacy/motor data
		data.rpm = 0;
		data.dutyCycleNow = 0;
		data.avgInputCurrent = 0;
		//data.inpVoltage = 0;

		// float package data
		data.floatPackageSupported = false;
		data.state = 255;
		data.fault = 0;
		data.isForward = true;

		usart_step = 0;
		commandIndex = 0;
		
		return;
	}
	
	switch(usart_step)
	{
		case 0:
			// Try the custom app command for the first 2 seconds then fall back to generic GET_VALUES
			if ((data.floatPackageSupported == false) && (Power_Time > VESC_BOOT_TIME * 2)) {
				Get_Vesc_Pack_Data(COMM_GET_VALUES);
			} else {
				uint8_t command = COMM_CUSTOM_APP_DATA;

#ifdef ADV
				if (commandIndex % 20 == 0) {
					// Sending charge info every 20th frame
					command = COMM_CHARGE_INFO;
				} else 
#endif
				if (lcmConfig.debug && commandIndex % 2 == 0) {
					// Send debug info every 2nd frame if enabled
					command = COMM_CUSTOM_DEBUG;
				}

				Get_Vesc_Pack_Data(command);

				if (commandIndex == 255) {
					commandIndex = 0;
				} else {
					commandIndex++;
				}
			}

			usart_step = 1;

			// We're expecting USART interrupts, so inhibit the WS2812 from refreshing since it disables interrupts
			WS2812_Inhibit = 1;
		break;
		
		case 1:
			if(VESC_RX_Flag == 1)
			{
				VESC_RX_Flag = 0;
				result = Protocol_Parse(VESC_RX_Buff);
				
				Vesc_Data_Ready = (result == 0);
				Usart_Time = 0;
				usart_step = 2;

				// The data has been received, so allow the WS2812 to refresh
				WS2812_Inhibit = 0;
			}
			else
			{
				usart_step = 3;
				Usart_Time = 0;
			}
		break;
			
		case 2:
			if(Usart_Time >= USART_INTERVAL_MS)
			{
				usart_step = 0;
			}				
		break;
			
		case 3:
			if(VESC_RX_Flag == 1)
			{
				usart_step = 1;
			}
			else if(Usart_Time >= USART_TIMEOUT_MS)
			{
				usart_step = 2;

				// The data was not received in time, so allow the WS2812 to refresh
				WS2812_Inhibit = 0;
			}
		break;
			
		default:
			
		break;
		
	}
	
}

/**************************************************
 * @brie   :ADC_Task()
 * @note   :Eavluate the 3 ADCs: 2 x sensors and 1 x current/voltage (multiplexed via LED1)
 **************************************************/
void ADC_Task(void)
{
	static uint8_t adc_step = 0;
	static uint16_t adc_charge_sum_ave = 0;
	static uint16_t adc1_val_sum_ave = 0;
	static uint16_t adc2_val_sum_ave = 0;
	
	switch(adc_step)
	{
		case 0:
			ADC_Time = 0;
			adc_step = 1;
		break;
		
		case 1:
			if(ADC_Time>100)
			{	
				ADC_Time = 0;
				
				adc_charge_sum_ave= Read_ADC_Value(3);
				adc1_val_sum_ave  = Read_ADC_Value(1);
				adc2_val_sum_ave  = Read_ADC_Value(2);
				
				ADC1_Val = (float)(adc1_val_sum_ave*0.0012890625);
				ADC2_Val = (float)(adc2_val_sum_ave*0.0012890625);

#ifdef ADV2
				Charge_Voltage = (float)(adc_charge_sum_ave*0.0008056640625);
#else
				if(V_I == 0)
				{
					if(Charge_Time>100)
					{
						Charge_Current = (float)(-0.008056640625*adc_charge_sum_ave+16.5);
					}
				}
				else
				{
					if(Charge_Time>100)
					{
						Charge_Voltage = (float)(adc_charge_sum_ave*0.0257080078125);
					}
				}
#endif
			}
		break;
			
	  default:
			
		break;
	}
}

/**************************************************
 * @brie   :VESC_State_Task()
 * @note   :proces VESC state when power is on and we're not charging
 **************************************************/
void VESC_State_Task(void)
{
	if ((Power_Flag != 2) || !Vesc_Data_Ready)
		return;

#ifdef ADV
	if (Charge_Flag > 0)
		return;
#endif

	Vesc_Data_Ready = false;

	// Not charging? Get voltage from VESC
	if (data.inpVoltage > 0) {
		CheckPowerLevel(data.inpVoltage/BATTERY_STRING);
	}

	if(data.dutyCycleNow < 0)
	{
		data.dutyCycleNow = -data.dutyCycleNow;
	}

	// Duty Cycle beep
	if ((lcmConfig.dutyBeep > 0) && (data.dutyCycleNow >= lcmConfig.dutyBeep))
	{
		Buzzer_Frequency = ((((uint8_t)(data.dutyCycleNow))*4)-220);
	}
	else
	{
		Buzzer_Frequency = 0;
	}
	// Don't buzz in wheel slip or flywheel mode
	if (data.state > RUNNING_UPSIDEDOWN) {
		Buzzer_Frequency = 0;
	}
	
	if(data.rpm<0)
	{
		data.rpm = -data.rpm;
	}
	
	if(data.state == RUNNING_FLYWHEEL) {
		WS2812_Display_Flag = 2;
		WS2812_Flag = 5;
		Buzzer_Frequency = 0;
	}
	else if(data.rpm<VESC_RPM)
	{
		if(ADC1_Val < 2.0 && ADC2_Val <2.0)
		{
			if((data.state == 0) || (data.state >= FAULT_ANGLE_PITCH))
			{   // disengaged board
				WS2812_Display_Flag = 1;
			}
		}
		else {
			WS2812_Display_Flag = 2;
			if(ADC1_Val > 2.9 && ADC2_Val > 2.9)
			{
				WS2812_Flag = 3;
			}
			else if(ADC1_Val >2.9)
			{
				WS2812_Flag = 1;
			}
			else
			{
				WS2812_Flag = 2;
			}
		}
	}
	else
	{
		// Add check for low voltage to force voltage display on WS2812!
		WS2812_Display_Flag = 2;
		WS2812_Flag = 4;	// Normal Riding!
	}
	
	// No movement and no ADCs? Shutdown after timeout (10-30min)
	if(ADC1_Val > 2.0 || ADC2_Val > 2.0 || data.rpm > 10)
	{
		Shutdown_Time_S = 0;
		Shutdown_Time_M = 0;
	}
	
#ifndef ADV2
	if(Shutdown_Time_S>60000)
	{
		Shutdown_Time_S = 0;
		
		Shutdown_Time_M++;
		if(Shutdown_Time_M >= SHUTDOWN_TIME)//lcmConfig.autoShutdownTime)
		{
			Power_Flag = 4;
			Power_Time = 0;
		}
	}

	if(((Shutdown_Time_M > 0) || (Shutdown_Time_S >= 10000)) && (lcmConfig.boardOff))
	{
		// After 10 seconds of idle we allow the board to be shut down via app
		Power_Flag = 4;
		Power_Time = 0;
	}
#endif
	lcmConfig.boardOff = false;
}

#ifdef ADV2
/**************************************************
 * @brie   :Charge_Detect_Task()
 * @note   :Detect charging state signalled by BMS
 **************************************************/
void Charge_Detect_Task(void)
{
    // NOTE from surfdado:
    // Somehow Charge_Voltage will be below 3.0V when charging starts
    // I guess ADC3 is just a signal from the BMS?
    if(Charge_Voltage >= CHARGING_VOLTAGE)
    {
        Charge_Flag = 0;
    }
    else if(data.inpVoltage >= FULL_VOLTAGE)
    {
        if(Charge_Time > 2000)
        {
            Charge_Flag = 3;
        }
    }
    else
    {
        Charge_Flag = 2;
        Charge_Time = 0;
    }
}
#endif
