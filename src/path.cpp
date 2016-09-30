#include <iostream>
#include "path.h"
#include "interval.h"
#include <deque>

using namespace std;

Path::Path(){ }

void Path::initialize(deque<interval> q)
{
    path = q;
    length = 0;
    start = q.begin()->start;
    //cout << "Paths starts at " << start;
    end = (*--q.end()).end;
    //cout << "Paths ends at " << end;
    for (std::deque<interval>::iterator it=path.begin(); it!=path.end(); ++it)
    {
	length += (*it).length;
    }
    end = path.back().end;
}

Path::~Path()
{
    path.clear();
}

void Path::add_start_interval(interval i)
{
    path.push_front(i);
    length += i.length;
    start = i.start;
}

void Path::add_end_interval(interval i)
{
    path.push_back(i);
    length += i.length;
    end = i.end;
}

void Path::print() const
{
    cout << "{";
    for (std::deque<interval>::const_iterator it=path.begin(); it!=path.end(); ++it)
    {
	it->print();
    }
    cout << "} ";
}
