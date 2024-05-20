#include <Arduino.h>
#include <SimpleCan.h>

static void handleCanMessage(FDCAN_RxHeaderTypeDef rxHeader, uint8_t *rxData);
static void init_CAN(void);
static void Button_Down(void);
//The correct pin sequence here is: Gnd, CAN L, CAN H, 5V
// pass in optional shutdown and terminator pins that disable transceiver and add 120ohm resistor respectively
SimpleCan can1(A_CAN_SHDN, A_CAN_TERM);
SimpleCan::RxHandler can1RxHandler(8, handleCanMessage);

FDCAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8];

void setup()
{
	pinMode(A_CAN_TERM, OUTPUT);
	digitalWrite(A_CAN_TERM, HIGH);
	Serial.begin(115200);
	Serial.println();

	while (!Serial)
		;

	pinMode(LED_BUILTIN, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(USER_BTN), Button_Down, LOW);

	delay(100);
	init_CAN();
}

void loop()
{
	// handleCanMessage();
}

static void init_CAN()
{
	Serial.println("Initializing CAN");
	Serial.println(can1.init(CanSpeed::Mbit1) == HAL_OK
					   ? "CAN: initialized."
					   : "CAN: error when initializing.");
	delay(10);
	FDCAN_FilterTypeDef sFilterConfig;

	// // Configure Rx filter
	sFilterConfig.IdType = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex = 0;
	sFilterConfig.FilterType = FDCAN_FILTER_MASK; //*!< Classic filter: FilterID1 = filter, FilterID2 = mask            */
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1 = 1 << 5; // filter
	sFilterConfig.FilterID2 = 1 << 5; // mask

	Serial.println("Setting filter");
	delay(10);
	can1.configFilter(&sFilterConfig);
	can1.configGlobalFilter(FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
	can1.activateNotification(&can1RxHandler);
	Serial.println("Starting CAN");
	Serial.println(can1.start() == HAL_OK
					   ? "CAN: started."
					   : "CAN: error when starting.");
	delay(10);
}

int dlcToLength(uint32_t dlc)
{
	int length = dlc >> 16;

	if (length >= 13)
	{
		return 32 + (13 - length) * 16;
	}
	else if (length == 12)
	{
		return 24;
	}
	else if (length >= 9)
	{
		return 12 + (9 - length) * 4;
	}
	return length;
}

static void handleCanMessage(FDCAN_RxHeaderTypeDef rxHeader, uint8_t *rxData)
{
	int byte_length = dlcToLength(rxHeader.DataLength);

	Serial.print("Received packet, id=0x");
	Serial.print(rxHeader.Identifier, HEX);

	Serial.print(", length=");
	Serial.print(byte_length);
	Serial.print("  ");
	for (uint8_t byte_index = 0; byte_index < byte_length; byte_index = byte_index + 1)
	{
		// Serial.print(" byte[");
		// Serial.print(byte_index);
		// Serial.print("]=");
		Serial.printf("%c", rxData[byte_index]);
		Serial.print(" ");
	}
	Serial.println();

	digitalToggle(LED_BUILTIN);
}

static void Button_Down()
{
	static uint32_t lasttime = 0;
	if(millis() - lasttime < 500) return;
	lasttime = millis();
	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	static uint8_t press_count = 0;

	press_count++;

	TxHeader.Identifier = 0x300;
	TxHeader.IdType = FDCAN_STANDARD_ID;
	TxHeader.TxFrameType = FDCAN_DATA_FRAME;
	TxHeader.DataLength = FDCAN_DLC_BYTES_2;
	TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
	TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
	TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	TxHeader.MessageMarker = 0;

	TxData[0] = 'c';
	TxData[1] = '0' + (press_count % 10);

	Serial.print("CAN: sending message ");
	Serial.println(can1.addMessageToTxFifoQ(&TxHeader, TxData) == HAL_OK
					   ? "was ok."
					   : "failed.");
}
