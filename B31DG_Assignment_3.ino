//defining I/O pins
#define t1out 1   //T1 digital out pin
#define t2in 2    //T2 digital in pin
#define t3in 3    //T3 digital in pin
#define t4in 4    //T4 analogue in pin
#define t4out 10  //T4 LED pin out
#define t6in 5    //T6 button pin in

/*  Task computation times:
---------------------------------------------------------------------------------------------------------
    Task1 - 284us
    Task2 - 2005us
    Task3 - 2005us
    Task4 - 57us
    Task5 - 1283us
---------------------------------------------------------------------------------------------------------    
*/

//Semaphores for Task2, 3, & 6
static SemaphoreHandle_t semt2;
static SemaphoreHandle_t semt3;
static SemaphoreHandle_t semt6;

//global struct for Task2, 3 and 5
struct frequencies {
  unsigned int freq1;
  unsigned int freq2;
} freq;

//global struct for Task6 and 7
struct buttoncounter {
  int presscount = 0;
} press;

//Task4 avg analogue value calculation
int a[4];
int counter = 0;
int avg = 0;

//Task 6 button debouncing
int newstate = 0;
int prevstate = 0;

void setup() {
  //set baud rate 9600
  Serial.begin(9600);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  //initialise output pins for Task1, 4 & 6
  pinMode(t1out,OUTPUT);
  pinMode(t4out,OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);

  //initialise input pins for Task2, 3, 4 & 6
  pinMode(t2in,INPUT);
  pinMode(t3in,INPUT);
  pinMode(t4in,INPUT);
  pinMode(t6in,INPUT);

  semt2 = xSemaphoreCreateBinary();
  semt3 = xSemaphoreCreateBinary();
  semt6 = xSemaphoreCreateBinary();

  xTaskCreatePinnedToCore(Task1, "Task 1", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(Task2, "Task 2", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(Task3, "Task 3", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(Task4, "Task 4", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(Task5, "Task 5", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(Task6, "Task 6", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(Task7, "Task 7", 4096, NULL, 1, NULL, 0);    
}

void loop(){}

void Task1(void *parameter){
  //Task1 pulse sequence
  while (1) {
  digitalWrite(t1out, HIGH);
  delayMicroseconds(200);
  digitalWrite(t1out, LOW);
  delayMicroseconds(50);
  digitalWrite(t1out, HIGH);
  delayMicroseconds(30);
  digitalWrite(t1out, LOW);
  vTaskDelay(4/portTICK_PERIOD_MS);
  }
}

void Task2(void *parameter){
  while (1) {
  //measure square wave freq (Hz)
  unsigned long freqhigh;
  freqhigh = pulseIn(t2in, HIGH, 3000);
  freq. freq1 = 1000000 / (freqhigh*2);
  xSemaphoreGive(semt2);
  vTaskDelay(20/portTICK_PERIOD_MS);
  }
}

void Task3(void *parameter){
  while (1) {
  //measure square wave freq (Hz)
  unsigned long freqhigh;
  freqhigh = pulseIn(t3in, HIGH, 3000);
  freq. freq2 = 1000000 / (freqhigh*2);
  xSemaphoreGive(semt3);
  vTaskDelay(8/portTICK_PERIOD_MS);
  }
}

void Task4(void *parameter){ 
  while (1) {
  //counter 0-3
  if(counter >= 3){
    counter = 0;
  }

  //read analogue signal on pin
  a[counter] = analogRead(t4in);

  //average value
  avg = (a[0]+a[1]+a[2]+a[3])/4;
  counter++;
  
  //LED high if above 2047 (max 4096)
  if(avg > 2400){
    digitalWrite(t4out, HIGH);
  }
  else{
    digitalWrite(t4out, LOW);
  }
  vTaskDelay(20/portTICK_PERIOD_MS);
  }
}

void Task5(void *parameter){
  while (1) {
  xSemaphoreTake(semt2, 0);
  xSemaphoreTake(semt3, 0);

  int mapf1;
  int mapf2;
  
  //setting outliers of freq range as 0 or 99
  mapf1 = map(freq. freq1, 333, 1000, 0, 99);
  mapf2 = map(freq. freq2, 500, 1000, 0, 99);

  mapf1 = constrain(mapf1, 0, 99);
  mapf2 = constrain(mapf2, 0, 99);
  
  //print T2 & 3 values
  Serial.printf("%d, %d \n", mapf1, mapf2);
  vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

void Task6(void *parameter){
  while (1) {    
  
  newstate = digitalRead(t6in);

  if(newstate != prevstate && newstate == LOW) {
    press. presscount++;
  }  
  prevstate = newstate;
  xSemaphoreGive(semt6);
  vTaskDelay(8/portTICK_PERIOD_MS);
  }
}

void Task7(void *parameter){
  while(1){
  xSemaphoreTake(semt6, 0);  
  if(press. presscount % 2 != 0) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else{
    digitalWrite(LED_BUILTIN, LOW);    
  }
  vTaskDelay(8/portTICK_PERIOD_MS);  
  }
}  