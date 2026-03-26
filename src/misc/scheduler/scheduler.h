#ifndef __SCHEDULER__H__ 
#define __SCHEDULER__H__ 

#include <vector>
#include <tuple>
#include <stack>
#include <map>
#include <deque>
#include <functional>
#include <unistd.h>
#include <math.h>
#include <Arduino.h>
#include <map>
#include <memory>

using namespace std;
using PrioDescriptionType = char;

#define MilliSeconds *1
#define Seconds *1000

class Task{
public:
    String name;
    function<void()> f;

    Task(String name, function<void()> f): name(name), f(f){}
    //Task(){}

    static Task Invalid;
};

class TimedTask: public Task{

public:
    bool __abort = false;
    bool __paused = false;
    enum TimedTaskType {PERIODIC, TIMEOUT};
    //the time of the task. For periodic tasks, this is the period. For timeout tasks, this is the delay until the task is executed
    uint period;
    TimedTaskType type = TimedTaskType::PERIODIC;
    PrioDescriptionType priority = 0;
    //the time count of this task. Is used for control when the task should be executed
    uint _timeCount = 0;

    //task life time, since it was created
    uint taskAge = 0;
    
    //used to prevent excessive task scheduling if scheduler has many pending tasks
    bool isCurrentlyScheduled = false;

    //finishes the task
    void abort(){ this->__abort = true; }

    void pause(){ this->__paused = true; }

    void resume(){ this->__paused = false; }

    std::map<String, String> tags;


    TimedTask(
        TimedTaskType type, 
        uint period, 
        function<void()> f, 
        PrioDescriptionType priority,
        String name = ""
    ):  Task(name, f),
        period(period), 
        type(type), 
        priority(priority)
        
    {}
};

#define shr_tmdtsk shared_ptr<TimedTask>





class DEFAULT_PRIORITIES{
public:
    static PrioDescriptionType VERY_HIGH;
    static PrioDescriptionType HIGH_;
    static PrioDescriptionType NORMAL;
    static PrioDescriptionType LOW_;
    static PrioDescriptionType VERY_LOW;
    static PrioDescriptionType WHO_KNOWS;

    //this vector contains a list of priorities and how much tasks of each priority should be executed in a round_of_tasks. The scheduler will use this list
    //to create the "taskVector"
    static vector<tuple<PrioDescriptionType, uint>> default_prio_list;

};



#define USE_THE_DEFAULT_PRIORITY 0

class Scheduler { 
private:
    PrioDescriptionType defaultPriority;
    std::map<PrioDescriptionType, std::deque<std::shared_ptr<Task>>> tasks;
    vector<PrioDescriptionType> runModel;

    std::vector<std::shared_ptr<TimedTask>> timedTasks;

    bool processingTimedTasks = false;
    uint processingTimedTaskLastTime = 0;

    void esp32_processTimeTasks(void);

    bool debug = false;

public: 
    /**
     * Create a scheduler with a priority system. The priority system is
     * created based in the variable 'prioritiesWithWeights'. The priority system
     * is based in a frequency which the scheduler will get the tasks. The
     * scheduler will try to run tasks with higher priority more frequenty than
     * tasks with less priority. If you constantly put mixed priority tasks in the
     * scheduerl, you will see that tasks with higher priorities are executed more
     * frequently than the less priority ones.
     * 
     * Bellow you can see the frequencies when a set of example priories are
     * executed:
     *               🢁 (priority)
     *               |
     * High priority:| hh hh  hh hh  hh hh  hh hh
     * Mid priority :|   m  m   m  m   m  m   m  m
     * low priority :|       l      l      l       l
     *            ---+------------------------------------> (time)
     *               |
     *
     * For the example above, the scheduler will take tasks following this vector
     * of priorities: 
     * [h, h, m, h, h, m, l, h, h, m, h, h, m, l, h, h, m, h, h, m, l, h, h, m, h, h, m, l]
    */
    Scheduler(vector<tuple<PrioDescriptionType, uint>> prioritiesWithWeights = DEFAULT_PRIORITIES::default_prio_list, PrioDescriptionType defaultPriority = DEFAULT_PRIORITIES::NORMAL);
    ~Scheduler();

    function<void(String)> log = [](String v){
        Serial.println(v);
    };

    /**
     * @brief Run a task. The task will be executed as soon as possible according to its priority
     * 
     * @param f An anonimous function. The task itself
     * @param name A opotional name for the task (helps in debuging, but have no impact in the scheduler)
     * @param priority Is the priority os the task, according with the priorities specified in the Scheduler class instantiation
     * @return Scheduler& A reference to the Scheduler itself, allow you to make things like 'scheduler.run([](){}).run([](){})'
     */
    void run(function<void()> f, String name = "", PrioDescriptionType priority = USE_THE_DEFAULT_PRIORITY);

    //when time is reached, the task is scheduled ( will be sent to the 'run' function)
    
    /**
     * @brief Schedule the task 'f' when the time specified in 'delay' passes.
     * 
     * @param delay Is the delay that should be waited before schedule the task. This argument should be given in milliseconds.
     * @param f An anonimous function. The task itself
     * @param name A opotional name for the task (helps in debuging, but have no impact in the scheduler)
     * @param priority Is the priority os the task, according with the priorities specified in the Scheduler class instantiation
     * @return Scheduler& A reference to the Scheduler itself, allow you to make things like 'scheduler.run([](){}).run([](){})'
     */
    void delayedTask(uint delay, function<void()> f, String name = "", PrioDescriptionType priority = USE_THE_DEFAULT_PRIORITY);

    /**
     * @brief Periodically schedules the task 'f'
     * 
     * @param period The period in which the task will be scheduled. This argument should be given in milliseconds.
     * @param f An anonimous function. The task itself
     * @param name A opotional name for the task (helps in debuging, but have no impact in the scheduler)
     * @param firstShotImediately if true, the first time the task will be schedule will be imediatelly after the call to 'periodicTask' method. 
     * @param priority Is the priority os the task, according with the priorities specified in the Scheduler class instantiation
     * @return Scheduler& A reference to the Scheduler itself, allow you to make things like 'scheduler.run([](){}).run([](){})'
     */
    std::shared_ptr<TimedTask> periodicTask(
        uint period, function<void()> f, 
        String name = "", 
        bool firstShotImediately = true, 
        PrioDescriptionType priority = USE_THE_DEFAULT_PRIORITY, 
        std::map<String, String> initialState = std::map<String, String>()
    );
    /**
     * @brief Periodically schedules the task 'f'. A control object will be passed to the 'f' function in every call, allowing you to control 
     * the task (like stoping or changing its scheduling period)
     * 
     * @param period The period in which the task will be scheduled. This argument should be given in milliseconds.
     * @param f An anonimous function. The task itself
     * @param name A opotional name for the task (helps in debuging, but have no impact in the scheduler)
     * @param firstShotImediately if true, the first time the task will be schedule will be imediatelly after the call to 'periodicTask' method. 
     * @param priority Is the priority os the task, according with the priorities specified in the Scheduler class instantiation
     * @return Scheduler& A reference to the Scheduler itself, allow you to make things like 'scheduler.run([](){}).run([](){})'
     */
    std::shared_ptr<TimedTask> periodicTask(
        uint period, 
        function<void(std::shared_ptr<TimedTask>)>f, 
        String name = "", 
        bool firstShotImediately = true, 
        PrioDescriptionType priority = USE_THE_DEFAULT_PRIORITY, 
        std::map<String, String> initialState = std::map<String, String>()
    );

    //make the scheduler work. As this scheduler was projected to run in the arduino/nodeMCU plataforms, you should call this function in the 'loop' function.
    void run_one_round_of_tasks();
private:
    vector<PrioDescriptionType> prepareTaskVector(vector<tuple<PrioDescriptionType, uint>> priorities, PrioDescriptionType emptyValue);
    size_t getNextValidPos(vector<PrioDescriptionType>& vec, double suggestedPos, PrioDescriptionType emptyValue);
    std::shared_ptr<Task> popNextTask(std::map<PrioDescriptionType, std::deque<std::shared_ptr<Task>>> &taskMap, PrioDescriptionType priority, bool &sucess);
    void processTimedTasks(uint elapsedTime);
    void processATimedTask(const std::shared_ptr<TimedTask>& task, uint elapsedTime);

}; 
 
#endif 
