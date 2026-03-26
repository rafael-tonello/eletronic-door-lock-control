#include  "scheduler.h" 


//these values are just diferente values for each priority. You can use numbers or characters. 
//Characters will help in debung. What matter here is that each priotity have a diferente value

char DEFAULT_PRIORITIES::VERY_HIGH = 1; // 'v'
char DEFAULT_PRIORITIES::HIGH_ = 2; // 'h'
char DEFAULT_PRIORITIES::NORMAL = 3; // 'n'
char DEFAULT_PRIORITIES::LOW_ = 4; //'l'
char DEFAULT_PRIORITIES::VERY_LOW = 5; //w
char DEFAULT_PRIORITIES::WHO_KNOWS = 6; //?
 
//this vector contains a list of priorities and how much tasks of each priority should be executed in a round_of_tasks. The scheduler will use this list
//to create the "taskVector"
vector<tuple<PrioDescriptionType, uint>> DEFAULT_PRIORITIES::default_prio_list = {
    {DEFAULT_PRIORITIES::VERY_HIGH, 40},
    {DEFAULT_PRIORITIES::HIGH_, 25},
    {DEFAULT_PRIORITIES::NORMAL, 15},
    {DEFAULT_PRIORITIES::LOW_, 8},
    {DEFAULT_PRIORITIES::VERY_LOW, 4},
    {DEFAULT_PRIORITIES::WHO_KNOWS, 1}
};

Task Task::Invalid = Task("invalid task", [](){});

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
 *            ---+------------------------------------> (period)
 *               |
 *
 * For the example above, the scheduler will take tasks following this vector
 * of priorities: 
 * [h, h, m, h, h, m, l, h, h, m, h, h, m, l, h, h, m, h, h, m, l, h, h, m, h, h, m, l]
*/
Scheduler::Scheduler(vector<tuple<PrioDescriptionType, uint>> prioritiesWithWeights, PrioDescriptionType defaultPriority)
{
    this->defaultPriority = defaultPriority;
    runModel = prepareTaskVector(prioritiesWithWeights, 0);     
    Serial.println("Run model setted with data");
    processingTimedTaskLastTime = millis();
}

void Scheduler::esp32_processTimeTasks(){

}
 
Scheduler::~Scheduler(void) 
{ 
    timedTasks.clear();
    tasks.clear();
} 
 
vector<PrioDescriptionType> Scheduler::prepareTaskVector(vector<tuple<PrioDescriptionType, uint>> priorities, PrioDescriptionType emptyValue)
{   
    uint totalSum = 0;
    for (auto &c: priorities)
            totalSum += std::get<1>(c);
    
    vector<PrioDescriptionType> result(totalSum, emptyValue);
    
    for (auto &c: priorities)
    {
        double stepSize = (double)totalSum/(double)(std::get<1>(c));
        double currPos = 0.f;

        //the math imprecisation can calculate the last position as .99999 of the toalSum, and some 
        //priorities can use an extra postion, so, the totalPosition is used to prevent this
        uint totalPositions = 0;

        while (currPos < totalSum && totalPositions < std::get<1>(c))
        {
            auto thePosition = getNextValidPos(result, currPos, emptyValue);
            if (thePosition >= 0 && thePosition < totalSum)
            {
                totalPositions++;
                result[thePosition] = std::get<0>(c);
                currPos += stepSize;
            }
            else
                log("Error preparing task vector: Invalid position from getNextValidPos function");
        }  
    }

    return result;
}

size_t Scheduler::getNextValidPos(vector<PrioDescriptionType>& vec, double suggestedPos, PrioDescriptionType emptyValue)
{
    size_t finalPos = (size_t)trunc(suggestedPos);
    while (finalPos < vec.size() && vec[finalPos] != emptyValue)
        finalPos++;
    
    return finalPos;
}

void Scheduler::run(function<void()> f, String name, PrioDescriptionType priority)
{
    if (priority == USE_THE_DEFAULT_PRIORITY)
        priority = defaultPriority;

    if (tasks.count(priority) == 0)
        tasks[priority] = {};
    
    tasks[priority].push_back(std::shared_ptr<Task>(new Task(name, f)));
}

std::shared_ptr<Task> Scheduler::popNextTask(std::map<PrioDescriptionType, std::deque<std::shared_ptr<Task>>> &taskMap, PrioDescriptionType priority, bool& sucess)
{
    // if (tasks.count(priority) == 0)
    // {
    //     log("Invalid priority description to pop a task");
    //     sucess = false;
    //     return Task();
    // }

    if (taskMap.count(priority) && taskMap[priority].size() > 0)
    {
        sucess = true;
        auto ret = taskMap[priority].front();
        taskMap[priority].pop_front();
        return ret;
    }
    else
    {
        sucess = false;
        return nullptr;
    }
}

void Scheduler::run_one_round_of_tasks()
{
    uint currentTime = millis();
    uint elapsedTime = currentTime - processingTimedTaskLastTime;
    processingTimedTaskLastTime = currentTime;

    if (elapsedTime > 0)
        processTimedTasks(elapsedTime);

    bool sucess;
    for (auto &c : this->runModel)
    {
        auto task = popNextTask(tasks, c, sucess);
        if (sucess)
        {
            if (this->debug && task->name != "")
                log("Running task: " + task->name);
            try{
                task->f();
            }
            catch (const std::exception& e)
            {
                log("Exception while running task '" + task->name + "': " + String(e.what()));
            }
            catch (...)
            {
                log("Unknown exception while running task '" + task->name + "'");
            }
            if (this->debug && task->name != "")
                log("Finish running task: " + task->name);
        }
        
    }
}

void Scheduler::processTimedTasks(uint elapsedTime)
{
    if (processingTimedTasks)
    {
        Serial.println("[error][scheduler] processTimedTasks called again before end last call");
        return;
    }

    processingTimedTasks = true;

    for (int c = timedTasks.size()-1; c >= 0; c--)
    {
        auto tsk = timedTasks[c];

        if (!tsk->__abort)
        {
            if (tsk->__paused)
                continue;
                
            processATimedTask(tsk, elapsedTime);
        }
        else {
            // Avoid removing while a task invocation that captured this object is still queued/running.
            if (!tsk->isCurrentlyScheduled)
                timedTasks.erase(timedTasks.begin() + c);
        }
    }

    processingTimedTasks = false;
}

/// @brief Check a timed task waiting period and run it when the time is reached
/// @param task The task to be processed
/// @param elaspedTime elasped time, in miliseconds, since the last call to this function
void Scheduler::processATimedTask(const std::shared_ptr<TimedTask>& task, uint elapsedTime)
{
    task->_timeCount += elapsedTime;
    if (task->_timeCount >= task->period )
    {
        task->taskAge += elapsedTime;
        if (!task->isCurrentlyScheduled)
        {
            task->isCurrentlyScheduled = true;
            
            this->run([task](){
                if (!task->name.isEmpty())
                    Serial.println("Running timed task: " + task->name);

                task->f();

                if (task->type == TimedTask::TimedTaskType::TIMEOUT)
                    task->abort();

                task->isCurrentlyScheduled = false;
            }, task->name, task->priority);

        }

        if (task->type == TimedTask::TimedTaskType::PERIODIC)
            task->_timeCount = 0;
    }
}


/**
 * @brief Schedule the task 'f' when the time specified in 'delay' passes.
 * 
 * @param delay Is the delay that should be waited before schedule the task. This argument should be given in milliseconds.
 * @param f An anonimous function. The task itself
 * @param name A opotional name for the task (helps in debuging, but have no impact in the scheduler)
 * @param priority Is the priority os the task, according with the priorities specified in the Scheduler class instantiation
 * @return Scheduler& A reference to the Scheduler itself, allow you to make things like 'scheduler.run([](){}).run([](){})'
 */
void Scheduler::delayedTask(uint delay, function<void()>f, String name, PrioDescriptionType priority)
{
    auto tsk = std::make_shared<TimedTask>(TimedTask::TimedTaskType::TIMEOUT, delay, f, priority, name);
    // tsk->f = [&, tsk, f](){
        // f();
    // };
    this->timedTasks.push_back(tsk);
}

std::shared_ptr<TimedTask> Scheduler::periodicTask(
    uint period, 
    function<void(std::shared_ptr<TimedTask>)>f, 
    String name, 
    bool firstShotImediately, 
    PrioDescriptionType priority, 
    std::map<String, String> initialState
)
{
    auto tsk = std::make_shared<TimedTask>(TimedTask::TimedTaskType::PERIODIC, period, [](){},priority, name);
    tsk->f = [=](){
        f(tsk);
    };

    if (firstShotImediately)
        tsk->_timeCount = tsk->period;
    else
        tsk->_timeCount = 0;

    this->timedTasks.push_back(tsk);

    tsk->tags = initialState;
    return tsk;
}

std::shared_ptr<TimedTask> Scheduler::periodicTask(
    uint period, 
    function<void()>f, 
    String name, 
    bool firstShotImediately, 
    PrioDescriptionType priority, 
    std::map<String, String> initialState
)
{
    return periodicTask(period, [f, name](std::shared_ptr<TimedTask> objCtrl){
        f();
    }, name, firstShotImediately, priority, initialState);
}
