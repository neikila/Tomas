#include <iostream>
#include <queue>
#include <math.h>
using namespace std;

void outputTime(double time, ostream& out = cout) {
    int days = (int)time / (60 * 24);
    int hours = ((int)time % (60 * 24)) / 60;
    double min = time - days * (60 * 24) - hours * 60;
    out << days << " d. " << hours << " h. " << min << " m. ";
}

class Machine;
class Detail;
ostream& operator << (ostream& out, Detail chain);
double Generate(double t) {
    return -1 * t * log((double)((rand() + 1) % 1000) / 1000.0);
}

enum State {
    inited,
    waitingProcessQueue,
    waitingRepairQueue,
    inProcess,
    terminateIt
};

ostream& operator << (ostream& out, State state) {
    switch (state) {
        case inited:
            out << "inited";
            break;
        case waitingProcessQueue:
            out << "waitingProcessQueue";
            break;
        case waitingRepairQueue:
            out << "waitingRepairQueue";
            break;
        case terminateIt:
            out << "terminateIt";
            break;
        case inProcess:
            out << "inProcess";
            break;
    }
    return out;
}

class Detail {
    int id;
public:
    static int idCounter;
    double time;
    int defectMadeCounter;
    State state;
    Machine *stanok;
    Detail(double t) :
            time(t),
            defectMadeCounter(0),
            state(inited) {
        id = ++idCounter;
        stanok = NULL;
    };

    void process(Machine & stanok) {
        state = inProcess;
        this->stanok = &stanok;
    }

    void wait() {
        if (state == inited)
            state = waitingProcessQueue;
        if (state == inProcess)
            state = waitingRepairQueue;
    }

    void terminate() { state = terminateIt; }

    State getState() { return state; }
    friend ostream& operator << (ostream& out, Detail chain);
};

int Detail::idCounter;

struct ComparatorByTime {
    bool operator()(const Detail & lch, const Detail & rch) const {
        return lch.time > rch.time;
    }
};

typedef priority_queue<Detail, vector<Detail>, ComparatorByTime> chainQueue;

class Machine {
private:
    double defectProbability;
    bool isFreeFlag;
    double advanceTime;
    int num;
public:
    Machine(double prob, double advTime, int numm = 0):
            defectProbability(prob),
            isFreeFlag(true),
            advanceTime(advTime),
            num(numm) {};

    int getNum() { return num; }

    bool process(Detail & detail) {
        if (isFreeFlag) {
            isFreeFlag = false;
            detail.time += Generate(advanceTime);
            detail.process(*this);
            return true;
        }
        return false;
    }

    bool hasBroken(Detail & detail) {
        if ((rand() % 101) < defectProbability * 100) {
            return false;
        }
        else {
            ++detail.defectMadeCounter;
            return true;
        }
    }

    bool isFree() { return isFreeFlag; }

    void free() { isFreeFlag = true; }
};

class Modelling {
    int totalDetails = 0;
    int completed = 0;
    int broken = 0;
    int wasted = 0;

    double currentTime;

    double timeBetweenDetails;

    Machine machine1;
    Machine machine2;

    chainQueue* cts;
    chainQueue cbs;
public:
    Modelling(): machine1(Machine(0.96, 30, 1)),
                 machine2(Machine(0.92, 50, 2)) {
        cts = new chainQueue();
        cout << "Size cts: " << cts->size() << endl;
        currentTime = 0.0;
        timeBetweenDetails = 35.0;
    }

    double getTotal() { return totalDetails; }
    double getCompleted() { return completed; }
    double getBroken() { return broken; }
    double getWasted() { return wasted; }
    double getTime() { return currentTime; }

    Machine * findStanok() {
        if (machine1.isFree())
            return &machine1;
        if (machine2.isFree())
            return &machine2;
        return NULL;
    }

    void generateNewDetail() {
        double time = currentTime + Generate(timeBetweenDetails);
        Detail newTempChain(time);
        cbs.push(newTempChain);
        ++totalDetails;
    }

    // false - nothing changed
    // true  - refresh CTS
    bool handle(Detail & detail) {
        bool result = false;
        switch (detail.getState()) {
            case inited:
                generateNewDetail();
            case waitingProcessQueue:
            {
                Machine * stanok = findStanok();
                if (stanok == NULL || !stanok->process(detail))
                    detail.wait();
                break;
            }
            case inProcess:
            {
                detail.stanok->free();
                result = true;
                if (!detail.stanok->hasBroken(detail) || detail.defectMadeCounter == 2) {
                    detail.terminate();
                    if (detail.defectMadeCounter < 2) {
                        ++completed;
                        if (detail.defectMadeCounter == 1)
                            ++broken;
                    } else {
                        ++wasted;
                    }
                    break;
                }
            }
            case waitingRepairQueue:
                if (!machine2.isFree() || !machine2.process(detail))
                    detail.wait();
                break;
            default:
                // Nothing to do
                break;
        }
        return result;
    }

    void lookThroughCTS() {
        chainQueue* temp = new chainQueue;
        while(!cts->empty()) {
            Detail detail = cts->top();
            cts->pop();
            bool shouldRestart = handle(detail);
            switch (detail.getState()) {
                case inProcess:
                    cbs.push(detail);
                    break;
                case waitingRepairQueue:
                case waitingProcessQueue:
                    temp->push(detail);
                    break;
                case terminateIt:
                    cout << (detail.defectMadeCounter != 2 ? "Complete" : "Waste") << endl;
                    cout << detail << endl;
                default:
                    cout << detail.getState() << endl;
                    break;
            }
            if (shouldRestart) {
                merge(temp);
                temp = new priority_queue <Detail, vector<Detail>, ComparatorByTime>();
            }
        }
        merge(temp);
    }

    void merge(chainQueue* temp) {
        while(!cts->empty()) {
            temp->push(cts->top());
            cts->pop();
        }
        delete cts;
        cts = temp;
    }

    void lookThroughCBS() {
        Detail detail = cbs.top();
        cbs.pop();
        currentTime = detail.time;
        cts->push(detail);
        while(!cbs.empty() && cbs.top().time == currentTime) {
            cts->push(cbs.top());
            cbs.pop();
        }
    }

    vector<Detail> getCBS() {
        vector<Detail> result;
        chainQueue duplicate;
        while(!cbs.empty()) {
            result.push_back(cbs.top());
            duplicate.push(cbs.top());
            cbs.pop();
        }
        cbs = duplicate;
        return result;
    }

    vector<Detail> getCTS() {
        vector<Detail> result;
        auto duplicate = new chainQueue();
        while(!cts->empty()) {
            result.push_back(cts->top());
            duplicate->push(cts->top());
            cts->pop();
        }
        delete cts;
        cts = duplicate;
        return result;
    }

    void print(bool printChain = false) {
        cout << "Current time: ";
        outputTime(getTime());
        cout << endl;
        cout << "Total = " << totalDetails << endl;
        cout << "Completed = " << completed << endl;
        cout << "Broken = " << broken << endl;
        cout << "Wasted = " << wasted << endl;

        cout << "CTS: = " << getCTS().size() << endl;
        if (printChain)
            for (auto const &detail: getCTS()) {
                cout << detail << endl;
            }

        cout << "CBS: = " << getCBS().size() << endl;
        if (printChain)
            for (auto const &detail: getCBS()) {
                cout << detail << endl;
            }
        cout << endl;
    }
};

ostream& operator << (ostream& out, Detail chain) {
    out << "Id: " << chain.id << ". Time: ";
    outputTime(chain.time, out);
    out << "State: " << chain.state;
    if (chain.stanok != NULL)
        out << " Machine #" << chain.stanok->getNum();
    return out;
}

int main() {
    srand(100);
    Detail::idCounter = 0;
    Modelling model;
    model.generateNewDetail();

    int limitDetails = 500;

    while (model.getWasted() + model.getCompleted() < limitDetails) {
        model.lookThroughCBS();
        model.lookThroughCTS();

        model.print(true);
    }
}

void testExponential() {
    double srVal = 35.0;
    double sum = 0;
    int amount = 100;
    for (int i = 0; i < amount; ++i) {
        double temp = Generate(srVal);
        cout << temp << endl;
        sum += temp;
    }
    cout << "sr: " << sum / amount << endl;
}
