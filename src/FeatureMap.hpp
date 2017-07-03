#ifndef FEATUREMAP_HPP
#define FEATUREMAP_HPP

#include "Config.hpp"
#include "Utilities.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <array>

#define USE_CACHE

class FeatureMap
{
public:
  typedef typename std::vector<std::string>::iterator iterator;
  typedef typename std::vector<std::string>::const_iterator const_iterator;

public:
  FeatureMap(const char* def_bases = "ACGUP");
  ~FeatureMap() { }

  std::vector<param_value_type> load_from_hash(const std::unordered_map<std::string, param_value_type>& hash);
  std::vector<param_value_type> read_from_file(const std::string& filename);
  void write_to_file(const std::string& filename, const std::vector<param_value_type>& vals) const;

private:
  void initialize_cache();

public:
  size_t find_key(const std::string& key) const;
  size_t insert_key(const std::string& key);

  iterator begin() { return keys_.begin(); }
  iterator end() { return keys_.end(); }
  const_iterator begin() const { return keys_.begin(); }
  const_iterator end() const { return keys_.end(); }

public:  
  // access to parameters
#if PARAMS_BASE_PAIR
  size_t find_base_pair(NUCL i, NUCL j) const;
  size_t insert_base_pair(NUCL i, NUCL j);
  void initialize_cache_base_pair();
#endif
#if PARAMS_BASE_PAIR_DIST
  size_t find_base_pair_dist_at_least(uint l) const;
  size_t insert_base_pair_dist_at_least(uint l);
  void initialize_cache_base_pair_dist_at_least();
#endif
#if PARAMS_TERMINAL_MISMATCH
  size_t find_terminal_mismatch(NUCL i1, NUCL j1, NUCL i2, NUCL j2) const;
  size_t insert_terminal_mismatch(NUCL i1, NUCL j1, NUCL i2, NUCL j2);
  void initialize_cache_terminal_mismatch();
#endif
#if PARAMS_HAIRPIN_LENGTH
  size_t find_hairpin_length_at_least(uint l) const;
  size_t insert_hairpin_length_at_least(uint l);
  void initialize_cache_hairpin_length_at_least();
#endif

  size_t find_hairpin_nucleotides(const std::vector<NUCL>& s, uint i, uint l) const;
  size_t insert_hairpin_nucleotides(const std::vector<NUCL>& s, uint i, uint l);

#if PARAMS_HELIX_LENGTH
  size_t find_helix_length_at_least(uint l) const;
  size_t insert_helix_length_at_least(uint l);
  void initialize_cache_helix_length_at_least();
#endif
#if PARAMS_ISOLATED_BASE_PAIR
  size_t find_isolated_base_pair() const;
  size_t insert_isolated_base_pair();
  void initialize_cache_isolated_base_pair();
#endif
#if PARAMS_INTERNAL_EXPLICIT
  size_t find_internal_explicit(uint i, uint j) const;
  size_t insert_internal_explicit(uint i, uint j);
  void initialize_cache_internal_explicit();
#endif
#if PARAMS_BULGE_LENGTH
  size_t find_bulge_length_at_least(uint l) const;
  size_t insert_bulge_length_at_least(uint l);
  void initialize_cache_bulge_length_at_least();
#endif
#if PARAMS_INTERNAL_LENGTH
  size_t find_internal_length_at_least(uint l) const;
  size_t insert_internal_length_at_least(uint l);
  void initialize_cache_internal_length_at_least();
#endif
#if PARAMS_INTERNAL_SYMMETRY
  size_t find_internal_symmetric_length_at_least(uint l) const;
  size_t insert_internal_symmetric_length_at_least(uint l);
  void initialize_cache_internal_symmetric_length_at_least();
#endif
#if PARAMS_INTERNAL_ASYMMETRY
  size_t find_internal_asymmetry_at_least(uint l) const;
  size_t insert_internal_asymmetry_at_least(uint l);
  void initialize_cache_internal_asymmetry_at_least();
#endif

  size_t find_internal_nucleotides(const std::vector<NUCL>& s, uint i, uint l, uint j, uint m) const;
  size_t insert_internal_nucleotides(const std::vector<NUCL>& s, uint i, uint l, uint j, uint m);

#if PARAMS_HELIX_STACKING
  size_t find_helix_stacking(NUCL i1, NUCL j1, NUCL i2, NUCL j2) const;
  size_t insert_helix_stacking(NUCL i1, NUCL j1, NUCL i2, NUCL j2);
  void initialize_cache_helix_stacking();
#endif
#if PARAMS_HELIX_CLOSING
  size_t find_helix_closing(NUCL i, NUCL j) const;
  size_t insert_helix_closing(NUCL i, NUCL j);
  void initialize_cache_helix_closing();
#endif
#if PARAMS_MULTI_LENGTH
  size_t find_multi_base() const;
  size_t insert_multi_base();
  void initialize_cache_multi_base();

  size_t find_multi_unpaired() const;
  size_t insert_multi_unpaired();
  void initialize_cache_multi_unpaired();

  size_t find_multi_paired() const;
  size_t insert_multi_paired();
  void initialize_cache_multi_paired();
#endif
#if PARAMS_DANGLE
  size_t find_dangle_left(NUCL i1, NUCL j1, NUCL i2) const;
  size_t insert_dangle_left(NUCL i1, NUCL j1, NUCL i2);
  void initialize_cache_dangle_left();

  size_t find_dangle_right(NUCL i1, NUCL j1, NUCL j2) const;
  size_t insert_dangle_right(NUCL i1, NUCL j1, NUCL j2);
  void initialize_cache_dangle_right();
#endif
#if PARAMS_EXTERNAL_LENGTH
  size_t find_external_unpaired() const;
  size_t insert_external_unpaired();
  void initialize_cache_external_unpaired();

  size_t find_external_paired() const;
  size_t insert_external_paired();
  void initialize_cache_external_paired();
#endif

private:
  const std::string def_bases_;
  size_t NBASES;
  std::unordered_map<std::string, size_t> hash_;
  std::vector<std::string> keys_;
  std::array<int, 256> is_base_;

  // cache
#ifdef USE_CACHE
#if PARAMS_BASE_PAIR
  VVI cache_base_pair_;
#endif
#if PARAMS_BASE_PAIR_DIST
  VI cache_base_pair_dist_at_least_;
#endif
#if PARAMS_TERMINAL_MISMATCH
  VVVVI cache_terminal_mismatch_;
#endif
#if PARAMS_HAIRPIN_LENGTH
  VI cache_hairpin_length_at_least_;
#endif
#if PARAMS_HELIX_LENGTH
  VI cache_helix_length_at_least_;
#endif
#if PARAMS_ISOLATED_BASE_PAIR
  int cache_isolated_base_pair_;
#endif
#if PARAMS_INTERNAL_EXPLICIT
  VVI cache_internal_explicit_;
#endif
#if PARAMS_BULGE_LENGTH
  VI cache_bulge_length_at_least_;
#endif
#if PARAMS_INTERNAL_LENGTH
  VI cache_internal_length_at_least_;
#endif
#if PARAMS_INTERNAL_SYMMETRY
  VI cache_internal_symmetric_length_at_least_;
#endif
#if PARAMS_INTERNAL_ASYMMETRY
  VI cache_internal_asymmetry_at_least_;
#endif
#if PARAMS_HELIX_STACKING
  VVVVI cache_helix_stacking_;
#endif
#if PARAMS_HELIX_CLOSING
  VVI cache_helix_closing_;
#endif
#if PARAMS_MULTI_LENGTH
  int cache_multi_base_;
  int cache_multi_unpaired_;
  int cache_multi_paired_;
#endif
#if PARAMS_DANGLE
  VVVI cache_dangle_left_;
  VVVI cache_dangle_right_;
#endif
#if PARAMS_EXTERNAL_LENGTH
  int cache_external_unpaired_;
  int cache_external_paired_;
#endif
#endif

};

#endif  // FEATUREMAP_HPP
