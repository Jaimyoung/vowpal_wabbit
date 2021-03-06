#include <stdio.h>
#include "../vowpalwabbit/vw.h"
#include "../vowpalwabbit/ezexample.h"
#include "libsearch.h"

struct wt {
  string word;
  uint32_t tag;
  wt(string w, uint32_t t) : word(w), tag(t) {}
};

class SequenceLabelerTask : public SearchTask< vector<wt>, vector<uint32_t> > {
  public:
  SequenceLabelerTask(vw& vw_obj)
      : SearchTask< vector<wt>, vector<uint32_t> >(vw_obj) {  // must run parent constructor!
    sch.set_options( Search::AUTO_HAMMING_LOSS | Search::AUTO_CONDITION_FEATURES );
    HookTask::task_data* d = sch.get_task_data<HookTask::task_data>();
    cerr << "num_actions = " << d->num_actions << endl;
  }

  // using vanilla vw interface
  void _run(Search::search& sch, vector<wt> & input_example, vector<uint32_t> & output) {
    output.clear();
    for (size_t i=0; i<input_example.size(); i++) {
      example* ex = VW::read_example(vw_obj, "1 |w " + input_example[i].word);
      action p  = Search::predictor(sch, i+1).set_input(*ex).set_oracle(input_example[i].tag).set_condition(i, 'p').predict();
      VW::finish_example(vw_obj, ex);
      output.push_back(p);
    }
  }

  // using ezexample
  void _run2(Search::search& sch, vector<wt> & input_example, vector<uint32_t> & output) {
    output.clear();
    for (size_t i=0; i<input_example.size(); i++) {
      ezexample ex(&vw_obj);
      ex(vw_namespace('w'))(input_example[i].word);  // add the feature
      action p  = Search::predictor(sch, i+1).set_input(*ex.get()).set_oracle(input_example[i].tag).set_condition(i, 'p').predict();
      output.push_back(p);
    }
  }

};

int main(int argc, char *argv[]) {
  // initialize VW as usual, but use 'hook' as the search_task
  vw& vw_obj = *VW::initialize("--search 4 --quiet --search_task hook --ring_size 1024");

  {
    // we put this in its own scope so that its destructor gets called
    // *before* VW::finish gets called; otherwise we'll get a
    // segfault :(. not sure what to do about this :(.
    SequenceLabelerTask task(vw_obj);
    vector<wt> data;
    vector<uint32_t> output;
    uint32_t DET = 1, NOUN = 2, VERB = 3, ADJ = 4;
    data.push_back( wt("the", DET) );
    data.push_back( wt("monster", NOUN) );
    data.push_back( wt("ate", VERB) );
    data.push_back( wt("a", DET) );
    data.push_back( wt("big", ADJ) );
    data.push_back( wt("sandwich", NOUN) );
    task.learn(data, output);
    task.learn(data, output);
    task.learn(data, output);
    task.predict(data, output);
    cerr << "output = [";
    for (size_t i=0; i<output.size(); i++) cerr << " " << output[i];
    cerr << " ]" << endl;
    cerr << "should have printed: 1 2 3 1 4 2" << endl;
  }
  
  VW::finish(vw_obj);
}














