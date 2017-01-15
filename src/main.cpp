#include "../config.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <string>
#include <stdexcept>
#include <ctime>
#include "cmdline.h"
#include "Config.hpp"
#include "Utilities.hpp"
#include "InferenceEngine.hpp"
#include "ParameterHash.hpp"
#include "SStruct.hpp"
#include "adagrad.hpp"

extern std::unordered_map<std::string, double> default_params_complementary;
extern std::unordered_map<std::string, double> default_params_noncomplementary;

class NGSfold
{
public:
  NGSfold() : train_mode_(false), mea_(false), gce_(false), validation_mode_(false) { }

  NGSfold& parse_options(int& argc, char**& argv);

  int run()
  {
    //return count_features();
    if (validation_mode_)
      return validate();
    if (train_mode_)
      return train();
    else
      return predict();
  }

private:
  int train();
  int predict();
  int validate();
  int count_features();
  std::pair<uint,uint> read_data(std::vector<SStruct>& data, const std::vector<std::string>& lists, int type) const;
  std::unordered_map<std::string,double> compute_gradients(const SStruct& s, const ParameterHash<double>* pm);
  

private:
  bool train_mode_;
  bool noncomplementary_;
  bool output_bpseq_;
  std::string out_file_;
  std::string param_file_;
  std::vector<std::string> data_str_list_;
  std::vector<std::string> data_unpaired_list_;
  std::vector<std::string> data_paired_list_;  
  std::vector<std::string> data_both_list_;  
  bool mea_;
  bool gce_;
  std::vector<float> gamma_;
  uint t_max_;
  uint t_burn_in_;
  float weight_weak_labeled_;
  float pos_w_;
  float neg_w_;
  bool per_bp_loss_;
  float lambda_;
  float eta0_;
  float scale_reactivity_;
  float threshold_unpaired_reactivity_;
  float threshold_paired_reactivity_;
  bool discretize_reactivity_;
  int verbose_;
  std::string out_param_;
  bool validation_mode_;
  bool use_constraints_;
  std::vector<std::string> args_;
};

NGSfold& 
NGSfold::parse_options(int& argc, char**& argv)
{
  gengetopt_args_info args_info;
  if (cmdline_parser(argc, argv, &args_info)!=0) exit(1);

  if (args_info.param_given)
    param_file_ = args_info.param_arg;

  if (args_info.train_given)
  {
    train_mode_ = true;
    out_file_ = args_info.train_arg;
  }

  if (args_info.mea_given)
  {
    mea_ = true;
    gamma_.resize(args_info.mea_given);
    for (uint i=0; i!=args_info.mea_given; ++i)
      gamma_[i] = args_info.mea_arg[i];
  }

  if (args_info.gce_given)
  {
    gce_ = true;
    gamma_.resize(args_info.gce_given);
    for (uint i=0; i!=args_info.gce_given; ++i)
      gamma_[i] = args_info.gce_arg[i];
  }

  if (args_info.structure_given)
  {
    data_str_list_.resize(args_info.structure_given);
    for (uint i=0; i!=args_info.structure_given; ++i)
      data_str_list_[i] = args_info.structure_arg[i];
  }

  if (args_info.unpaired_reactivity_given)
  {
    data_unpaired_list_.resize(args_info.unpaired_reactivity_given);
    for (uint i=0; i!=args_info.unpaired_reactivity_given; ++i)
      data_unpaired_list_[i] = args_info.unpaired_reactivity_arg[i];
  }

  if (args_info.paired_reactivity_given)
  {
    data_paired_list_.resize(args_info.paired_reactivity_given);
    for (uint i=0; i!=args_info.paired_reactivity_given; ++i)
      data_paired_list_[i] = args_info.paired_reactivity_arg[i];
  }

  if (args_info.both_reactivity_given)
  {
    data_both_list_.resize(args_info.both_reactivity_given);
    for (uint i=0; i!=args_info.both_reactivity_given; ++i)
      data_both_list_[i] = args_info.both_reactivity_arg[i];
  }

  if (args_info.out_param_given)
    out_param_ = args_info.out_param_arg;

  noncomplementary_ = args_info.noncomplementary_flag==1;
  output_bpseq_ = args_info.bpseq_flag==1;
  t_max_ = args_info.max_iter_arg;
  t_burn_in_ = args_info.burn_in_arg;
  weight_weak_labeled_ = args_info.weight_weak_label_arg;
  pos_w_ = args_info.pos_w_arg;
  neg_w_ = args_info.neg_w_arg;
  lambda_ = args_info.lambda_arg;
  eta0_ = args_info.eta_arg;
  scale_reactivity_ = args_info.scale_reactivity_arg;
  threshold_unpaired_reactivity_ = args_info.threshold_unpaired_reactivity_arg;
  threshold_paired_reactivity_ = args_info.threshold_paired_reactivity_arg;
  per_bp_loss_ = args_info.per_bp_loss_flag==1;
  discretize_reactivity_ = args_info.discretize_reactivity_flag==1;
  verbose_ = args_info.verbose_arg;
  use_constraints_ = args_info.constraints_flag==1;
  validation_mode_ = args_info.validate_flag==1;

  srand(args_info.random_seed_arg<0 ? time(0) : args_info.random_seed_arg);

  if ((!train_mode_ && args_info.inputs_num==0) ||
      (train_mode_ && data_str_list_.empty() && data_unpaired_list_.empty() && data_paired_list_.empty() && args_info.inputs_num==0)) 
  {
    cmdline_parser_print_help();
    cmdline_parser_free(&args_info);
    exit(1);
  }

  args_.resize(args_info.inputs_num);
  for (uint i=0; i!=args_info.inputs_num; ++i)
    args_[i]=args_info.inputs[i];

  cmdline_parser_free(&args_info);

  return *this;
}

std::pair<uint,uint>
NGSfold::
read_data(std::vector<SStruct>& data, const std::vector<std::string>& lists, int type) const
{
  std::pair<uint,uint> pos = std::make_pair(data.size(), data.size());
  for (auto l: lists)
  {
    std::ifstream is(l.c_str());
    if (!is) throw std::runtime_error(std::string(strerror(errno)) + ": " + l);
    std::string f;
    while (is >> f)
    {
      data.emplace_back(f, type);
      if (type!=SStruct::NO_REACTIVITY && discretize_reactivity_)
        data.back().DiscretizeReactivity(threshold_unpaired_reactivity_, threshold_paired_reactivity_);
    }
  }
  pos.second = data.size();
  return pos;
}

std::unordered_map<std::string,double>
NGSfold::
compute_gradients(const SStruct& s, const ParameterHash<double>* pm)
{
  double starting_time = GetSystemTime();
  std::unordered_map<std::string,double> grad;
  int np=1;

  // count the occurence of parameters in the correct structure
  auto max_single_length = DEFAULT_C_MAX_SINGLE_LENGTH;
  if (s.GetType() == SStruct::NO_REACTIVITY)
    max_single_length = std::max<int>(s.GetLength()/2., DEFAULT_C_MAX_SINGLE_LENGTH);
  InferenceEngine<double> inference_engine1(noncomplementary_, max_single_length);
  inference_engine1.LoadValues(pm);
  inference_engine1.LoadSequence(s);
  std::cout << s.GetNames()[0] << std::endl;
  if (s.GetType() == SStruct::NO_REACTIVITY || discretize_reactivity_)
  {
    inference_engine1.UseConstraints(s.GetMapping());
    inference_engine1.ComputeViterbi();
    if (per_bp_loss_)
      for (auto m : s.GetMapping())
        if (m != SStruct::UNKNOWN && m != SStruct::UNPAIRED) ++np;
  }
  else
  {
    inference_engine1.UseSoftConstraints(s.GetReactivityUnpair(), s.GetReactivityPair(), 
                                         threshold_unpaired_reactivity_, threshold_paired_reactivity_, scale_reactivity_);
    inference_engine1.ComputeViterbi();
    if (per_bp_loss_)
    {
      SStruct solution(s);
      solution.SetMapping(inference_engine1.PredictPairingsViterbi());
      for (auto m : solution.GetMapping())
        if (m != SStruct::UNKNOWN && m != SStruct::UNPAIRED) ++np;
    }
  }

  auto loss1 = inference_engine1.GetViterbiScore();
  auto corr = inference_engine1.ComputeViterbiFeatureCounts();
  for (auto e : corr)
    grad.insert(std::make_pair(e.first, 0.0)).first->second -= e.second;


  // count the occurence of parameters in the predicted structure
  InferenceEngine<double> inference_engine0(noncomplementary_);
  inference_engine0.LoadValues(pm);
  inference_engine0.LoadSequence(s);
  if (s.GetType() == SStruct::NO_REACTIVITY)
    inference_engine0.UseLossBasePair(s.GetMapping(), pos_w_/np, neg_w_/np);
  else if (discretize_reactivity_)
    inference_engine0.UseLossPosition(s.GetMapping(), pos_w_/np, neg_w_/np);
  else
    inference_engine0.UseLossReactivity(s.GetReactivityUnpair(), s.GetReactivityPair(), pos_w_/np, neg_w_/np);

  inference_engine0.ComputeViterbi();
  auto loss0 = inference_engine0.GetViterbiScore();
  auto pred = inference_engine0.ComputeViterbiFeatureCounts();
  for (auto e : pred)
    grad.insert(std::make_pair(e.first, 0.0)).first->second += e.second;


  if (verbose_>0)
  {
    std::cout << "Seq: " << s.GetNames()[0] << ", "
              << "Loss: " << loss0-loss1 << ", "
              << "pos_w: " << pos_w_/np << ", " << "neg_w: " << neg_w_/np << ", "
              << "Time: " << GetSystemTime() - starting_time << "sec" << std::endl;
  }
  if (verbose_>1)
  {
    SStruct solution1(s);
    solution1.SetMapping(inference_engine1.PredictPairingsViterbi());
    solution1.WriteParens(std::cout);

    SStruct solution0(s);
    solution0.SetMapping(inference_engine0.PredictPairingsViterbi());
    solution0.WriteParens(std::cout);

    std::cout << std::endl;
  }

  return std::move(grad);
}

int
NGSfold::train()
{
  // read traing data
  std::vector<SStruct> data;
  auto pos_str = read_data(data, data_str_list_, SStruct::NO_REACTIVITY);
  /*auto pos_unpaired =*/ read_data(data, data_unpaired_list_, SStruct::REACTIVITY_UNPAIRED);
  /*auto pos_paired =*/ read_data(data, data_paired_list_, SStruct::REACTIVITY_PAIRED);
  auto pos_both = read_data(data, data_both_list_, SStruct::REACTIVITY_BOTH);

  //AdaGradRDAUpdater optimizer(eta0_, lambda_);
  AdaGradFobosUpdater optimizer(eta0_, lambda_);
  ParameterHash<double> pm;
  if (!param_file_.empty())
  {
    pm.ReadFromFile(param_file_);
    optimizer.read_from_file(param_file_);
  }

  // run max-margin training
  for (uint t=0, k=0; t!=t_max_; ++t)
  {
    if (verbose_>0)
      std::cout << std::endl << "=== Epoch " << t << " ===" << std::endl;

    std::vector<uint> idx(t<t_burn_in_ ? pos_str.second : pos_both.second);
    std::iota(idx.begin(), idx.end(), 0);
    std::random_shuffle(idx.begin(), idx.end());

    for (auto i : idx)
    {
      // weight for this instance
      auto w = i<pos_str.second ? 1.0 : weight_weak_labeled_;
      // gradient
      auto grad = compute_gradients(data[i], &pm);
      // update
      for (auto g : grad)
        if (g.second!=0.0)
          optimizer.update(g.first, pm.get_by_key(g.first), g.second*w);
      // regularize
      for (auto p=pm.begin(); p!=pm.end(); )
      {
        optimizer.regularize(p->first, p->second);
        if (p->second==0.0)
          p = pm.erase(p);
        else
          ++p;
      }
      optimizer.proceed_timestamp();

      if (!out_param_.empty())
      {
        //pm.WriteToFile(SPrintF("%s/%d.param", out_param_.c_str(), k++));
        optimizer.write_to_file(SPrintF("%s/%d.param", out_param_.c_str(), k++), &pm);
      }
    }
  }

  //pm.WriteToFile(out_file_);
  optimizer.write_to_file(out_file_, &pm);

  return 0;
}

int
NGSfold::predict()
{
  // set parameters
  ParameterHash<double> pm;

  if (!param_file_.empty())
    pm.ReadFromFile(param_file_);
  else if (noncomplementary_)
    pm.LoadFromHash(default_params_noncomplementary);
  else
    pm.LoadFromHash(default_params_complementary);
  
  // predict ss
  InferenceEngine<double> inference_engine(noncomplementary_);
  inference_engine.LoadValues(&pm);
  for (auto s : args_)
  {
    SStruct sstruct;
    sstruct.Load(s);
    SStruct solution(sstruct);
    inference_engine.LoadSequence(sstruct);
    //sstruct.WriteParens(std::cout); // for debug
    //std::cout << std::endl;
    if (use_constraints_)
      inference_engine.UseConstraints(sstruct.GetMapping());
    if (!mea_ && !gce_)
    {
      inference_engine.ComputeViterbi();
      solution.SetMapping(inference_engine.PredictPairingsViterbi());
    }
    else
    {
      inference_engine.ComputeInside();
      inference_engine.ComputeOutside();
      inference_engine.ComputePosterior();
      if (mea_)
        solution.SetMapping(inference_engine.PredictPairingsPosterior<0>(gamma_[0]));
      else
        solution.SetMapping(inference_engine.PredictPairingsPosterior<1>(gamma_[0]));
    }
    if (output_bpseq_)
      solution.WriteBPSEQ(std::cout);
    else
      solution.WriteParens(std::cout);
  }

  return 0;
}

int
NGSfold::validate()
{
  // set parameters
  ParameterHash<double> pm;
  if (!param_file_.empty())
    pm.ReadFromFile(param_file_);
  else if (noncomplementary_)
    pm.LoadFromHash(default_params_noncomplementary);
  else
    pm.LoadFromHash(default_params_complementary);
  
  for (auto s : args_)
  {
    SStruct sstruct;
    sstruct.Load(s);
    SStruct solution(sstruct);
    InferenceEngine<double> inference_engine(noncomplementary_, 
                                             std::max<int>(sstruct.GetLength()/2., DEFAULT_C_MAX_SINGLE_LENGTH));
    inference_engine.LoadValues(&pm);
    inference_engine.LoadSequence(sstruct);
    inference_engine.UseConstraints(sstruct.GetMapping());
    inference_engine.ComputeViterbi();
    auto score = inference_engine.GetViterbiScore();
    std::cout << sstruct.GetNames()[0] << " " 
              << (score>NEG_INF ? "OK" : "NG") << std::endl;
    if (score==NEG_INF)
    {
      sstruct.WriteParens(std::cout); // for debug
      std::cout << std::endl;
    }  
  }

  return 0;
}

#if 0
// for debug
int
NGSfold::count_features()
{
  // set parameters
  ParameterHash<double> pm;
  if (!param_file_.empty())
    pm.ReadFromFile(param_file_);
  else if (noncomplementary_)
    pm.LoadFromHash(default_params_noncomplementary);
  else
    pm.LoadFromHash(default_params_complementary);
  
  std::unordered_map<std::string, double> cnt;
  for (auto s : args_)
  {
    SStruct sstruct;
    sstruct.Load(s);
    SStruct solution(sstruct);
    InferenceEngine<double> inference_engine(noncomplementary_, 
                                             std::max<int>(sstruct.GetLength()/2., DEFAULT_C_MAX_SINGLE_LENGTH));
    inference_engine.LoadValues(&pm);
    inference_engine.LoadSequence(sstruct);
    inference_engine.UseConstraints(sstruct.GetMapping());
    inference_engine.ComputeViterbi();
    auto corr = inference_engine.ComputeViterbiFeatureCounts();
    for (auto e: corr)
      if (e.second!=0.0)
        cnt.insert(std::make_pair(e.first, 0.0)).first->second += e.second;
  }

  std::vector<std::string> keys;
  for (auto e : cnt)
    keys.emplace_back(e.first);
  std::sort(keys.begin(), keys.end());
  for (const auto& k : keys)
    std::cout << k << " " << cnt.find(k)->second << std::endl;

  return 0;
}
#endif

int
main(int argc, char* argv[])
{
  try {
    return NGSfold().parse_options(argc, argv).run();
  } catch (const char* str) {
    std::cerr << str << std::endl;
  } catch (std::runtime_error e) {
    std::cerr << e.what() << std::endl;
  }
  return -1;
}
