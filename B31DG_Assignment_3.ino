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
unsigned long tm;
unsigned long newtm;
//monitoring
unsigned long t1count = 0;
unsigned long t2count = 0;
unsigned long t3count = 0;
unsigned long t4count = 0;
unsigned long t5count = 0;
unsigned long t6count = 0;
unsigned long t7count = 0;

//Semaphore for Task2, 3, & 5
static SemaphoreHandle_t semfreq;


//global struct for Task2, 3 and 5
struct frequencies {
  unsigned int freq1;
  unsigned int freq2;
} freq;

//queue for Task6 and 7
QueueHandle_t buttonqueue;

//Task4 avg analogue value calculation
int a[4];
int counter = 0;
int avg = 0;

//Task 6 button debouncing
int newstate = 0;
int prevstate = 0;
int presscount = 1;

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

  semfreq = xSemaphoreCreateBinary();

  xTaskCreatePinnedToCore(TaskMonitor, "TaskMonitor", 4096, NULL, 5, NULL, 0); 
  xTaskCreatePinnedToCore(Task1, "Task 1", 4096, NULL, 4, NULL, 0);
  xTaskCreatePinnedToCore(Task2, "Task 2", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(Task3, "Task 3", 4096, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(Task4, "Task 4", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(Task5, "Task 5", 4096, NULL, 4, NULL, 0);
  xTaskCreatePinnedToCore(Task6, "Task 6", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(Task7, "Task 7", 4096, NULL, 2, NULL, 0);   
}

void loop(
  
){}

void Task1(void *parameter){
  //Task1 pulse sequence
  while (1) {
  TickType_t TaskBegin = xTaskGetTickCount();
  digitalWrite(t1out, HIGH);
  delayMicroseconds(200);
  digitalWrite(t1out, LOW);
  delayMicroseconds(50);
  digitalWrite(t1out, HIGH);
  delayMicroseconds(30);
  digitalWrite(t1out, LOW);
  t1count++;
  vTaskDelayUntil(&TaskBegin, 4/portTICK_PERIOD_MS);
  }
}

void Task2(void *parameter){
  while (1) {
  //measure square wave freq (Hz)
  TickType_t TaskBegin = xTaskGetTickCount();
  unsigned long freqhigh;
  freqhigh = pulseIn(t2in, HIGH, 3000);
  xSemaphoreTake(semfreq, 0);
  freq. freq1 = 1000000 / (freqhigh*2);
  xSemaphoreGive(semfreq);
  t2count++;
  vTaskDelayUntil(&TaskBegin, 20/portTICK_PERIOD_MS);
  }
}

void Task3(void *parameter){
  while (1) {
  //measure square wave freq (Hz)
  TickType_t TaskBegin = xTaskGetTickCount();
  unsigned long freqhigh;
  freqhigh = pulseIn(t3in, HIGH, 3000);
  xSemaphoreTake(semfreq, 0);
  freq. freq2 = 1000000 / (freqhigh*2);
  xSemaphoreGive(semfreq);
  t3count++;
  vTaskDelayUntil(&TaskBegin, 8/portTICK_PERIOD_MS);
  }
}

void Task4(void *parameter){ 
  while (1) {
  //counter 0-3
  TickType_t TaskBegin = xTaskGetTickCount();
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
  t4count++;
  vTaskDelayUntil(&TaskBegin, 20/portTICK_PERIOD_MS);
  }
}

void Task5(void *parameter){
  while (1) {
  TickType_t TaskBegin = xTaskGetTickCount();  

  int mapf1;
  int mapf2;
  
  //setting outliers of freq range as 0 or 99
  xSemaphoreTake(semfreq, 0);
  mapf1 = map(freq. freq1, 333, 1000, 0, 99);
  mapf2 = map(freq. freq2, 500, 1000, 0, 99);
  xSemaphoreGive(semfreq);

  mapf1 = constrain(mapf1, 0, 99);
  mapf2 = constrain(mapf2, 0, 99);
  
  //print T2 & 3 values
  Serial.printf("%d, %d \n", mapf1, mapf2);
  t5count++;
  vTaskDelayUntil(&TaskBegin, 100/portTICK_PERIOD_MS);
  }
}

void Task6(void *parameter){
  while (1) { 
  TickType_t TaskBegin = xTaskGetTickCount(); 
  unsigned int press; 
  //create queue to send LED toggle information  
  buttonqueue = xQueueCreate(10, sizeof(int));
  
  //debounce ny checking previous 10 inputs are all high
  for(int i=0; i<10; i++){
  newstate += digitalRead(t6in);
  }
  newstate = newstate/10;

  //increment presscount on falling edge of button press
  if(newstate != prevstate && newstate == LOW) {
    presscount++; 
  } 

  //usind modulo 2 to determine if preeecount is even or odd and adding value to the queue
  if(presscount%2 == 0){
    press = 1;    
    xQueueSend(buttonqueue, &press, 0);
  } 
  else{
    press = 0;
    xQueueSend(buttonqueue, &press, 0);
  }
  prevstate = newstate;
  
  t6count++;
  vTaskDelayUntil(&TaskBegin, 20/portTICK_PERIOD_MS);
  }
}

void Task7(void *parameter){
  while(1){
  TickType_t TaskBegin = xTaskGetTickCount(); 
  int LEDstate; 
  //recieve LEDstate from queue
  xQueueReceive(buttonqueue, &LEDstate, 0); 
  //toggle LED
  digitalWrite(LED_BUILTIN, LEDstate);

  t7count++;
  vTaskDelayUntil(&TaskBegin, 20/portTICK_PERIOD_MS);  
  }
}  

void TaskMonitor(void *parameter){
  while(1){
  //begin 10 second timer imediately and print task iterations    
  vTaskDelay(10000/portTICK_PERIOD_MS);
  Serial.print(t1count);
  Serial.print(",");
  Serial.print(t2count);
  Serial.print(",");
  Serial.print(t3count);
  Serial.print(",");
  Serial.print(t4count);
  Serial.print(",");
  Serial.print(t5count);
  Serial.print(",");
  Serial.print(t6count);
  Serial.print(",");
  Serial.println(t7count);
  t1count = t2count = t3count = t4count = t5count = t6count = t7count = 0;
  }
}  