/*ident "@(#)ccsdemos:sc_demos/Graph_demo.C	1.4" */
// This corresponds (with a few revisions) to the Appendix A example in
// the Graph tutorial in the SC Programmer's Guide

#include <Graph.h> 
#include <Graph_alg.h> 
#include <String.h> 
#include <Map.h>
#include <iostream.h>

using namespace SCO_SC;

class Module; 
typedef Module* Mptr; 

#define mod(s) (Module::m[s])

Graphdeclare1(Product,Module,Transport_Time) 

class Product: public Graph { 
public:     
	Product() : 
	Graph() {}     
	derivedGraph(Product,Module,Transport_Time)    
}; 

class Module: public Vertex { 
public:     
	static Map<String,Mptr> m;     
	String id;
	Module(String s) : Vertex(), id(s) { m[s] = this; }     
	derivedVertex(Product,Module,Transport_Time)
}; 

Map<String,Mptr> Module::m; 

class Transport_Time: public Edge { 
public:     
	int ttime;     
	Transport_Time(Module* m1, Module* m2, int time) :         
		Edge(m1, m2), ttime(time) {}     
	derivedEdge(Product,Module,Transport_Time)
}; 

Graphdeclare2(Product,Module,Transport_Time) 
Graph_algdeclare(Product,Module,Transport_Time) 
Graph_algimplement(Product,Module,Transport_Time) 

Set_of_p<Module> 
listp_to_setp(List_of_p<Module> lp) {     
	Set_of_p<Module> sm;     
	List_of_piter<Module> lpi (lp);     
	Module* mp;     
	while (lpi.next(mp))
		sm.insert(mp);     
	return sm;     
} 

void print(Set_of_p<Module> mset) {
	Set_of_piter<Module> sm (mset);     
	Module* mp;     
	while (mp = sm.next())
		cout << mp->id << endl;
} 

static Product prod; 
static List_of_p<Module> dfunc_list; 

int dfunc(Module* m) {     
	Set_of_p<Transport_Time> s (m->out_edges_g(prod));
	Set_of_piter<Transport_Time> si (s);
	Transport_Time* t;     
	while (t = si.next())
         if (!t->ttime) {             
		dfunc_list.put(m);             
		break;
  		}     
	return 1;     
} 

int d2func(Module* m) {     
	Set_of_p<Transport_Time> s (m->out_edges_g(prod));
	Set_of_piter<Transport_Time> si (s);
	Transport_Time* t;     
	while ( t = si.next())
         if (!t->ttime)
             return 0;     
	return 1;     
}

int main() {     
	Product widget;     
	widget.insert(
         new Transport_Time(     
	    new Module("A"), new Module("D"), 5));     
	widget.insert(
         new Transport_Time(
             mod("A"), new Module("F"), 1));     
	widget.insert(
         new Transport_Time(
              mod("D"), new Module("C"), 2));     
	widget.insert(
         new Transport_Time(
             mod("D"), new Module("J"), 0));
	widget.insert(
         new Transport_Time(
             new Module("M"), mod("J"), 0));     
	widget.insert(
         new Transport_Time(
             mod("C"), new Module("B"), 0));     
	widget.insert(
         new Transport_Time(           
	    mod("J"), new Module("K"), 6));     
	widget.insert(
         new Transport_Time(
	    mod("J"), mod("B"), 7));     
	widget.insert(
         new Transport_Time(
             mod("K"), new Module("Z"), 8));     
	widget.insert(
         new Transport_Time(
             mod("B"), mod("Z"), 9));     

	// Where is module K needed?

        cout << "Where is module K needed?" << endl;
	List_of_p<Module> mlist = dfs(widget, mod("K"));     
	Set_of_p<Module> m1set = listp_to_setp(mlist);
	print(m1set);     

	prod = widget;     

	// What modules are immediately needed
        // to compose module K?
        cout << "What modules are immediately needed"
	     << " to compose module K?" << endl;     
	Set_of_p<Module> mset;
         	// create a pointer set to hold the answer set
	Set_of_p<Transport_Time> s (mod("K")->in_edges_g(widget));
	Set_of_piter <Transport_Time> si (s);
	Transport_Time* t;     
	while (t = si.next())  
		mset.insert(t->src());     
	print(mset);     

	// Example in Section 5.1
	List_of_p<Module> m6list = dfs(widget, mod("A"), d2func);
	cout << "First 0 edged Vertex found: " << m6list.tail()->id  << endl;

	List_of_p<Module> m5list = dfs(widget, mod("A"), dfunc);     
	cout << "List of 0 edged Vertices:" << endl;     
	List_of_piter<Module> lim5 (dfunc_list);     
	Module* mp;     
	while (lim5.next(mp))
		cout << mp->id << endl;

	// clean up     
	Set_of_p<Module> s2 (widget.vertices());
	Set_of_piter<Module> si2 (s2);
	Module* mp2;     
	while (mp2 = si2.next()) {         
		Set_of_p<Transport_Time> s3 (mp2->edges());
		Set_of_piter<Transport_Time> si3 (s3);
		Transport_Time* t;
		while (t = si3.next())       
		      delete t;         
		delete  mp2;         
	}
}
