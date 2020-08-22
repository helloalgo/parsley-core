#include <stack>
#include <vector>
#include <iostream>
using namespace std;

stack<int> towers;
vector<int> input;
int cnt,tmp;
int main()
{
    cin>>cnt;
    for(int i=0;i<cnt;i++) {
        cin>>tmp;
        input.push_back(tmp);
    }
    for(int i=0;i<cnt;i++) {
        while(!towers.empty() && input[towers.top()]<input[i]) towers.pop();
        cout<<(towers.empty()?0:towers.top()+1)<<" ";
        towers.push(i);
    }
}