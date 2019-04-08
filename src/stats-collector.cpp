#include <assert.h>
#include <unordered_map>

#include "stats-collector.hpp"
#include "drachtio.h"
#include "controller.hpp"

#include <prometheus/exposer.h>
#include <prometheus/registry.h>

using namespace prometheus;

namespace drachtio {

  class StatsCollector::PromImpl {
  public:
    typedef std::unordered_map<string, std::shared_ptr<Family<Counter> > > mapCounter_t;
    typedef std::unordered_map<string, std::shared_ptr<Family<Gauge> > > mapGauge_t;

    using Quantiles = std::vector<detail::CKMSQuantiles::Quantile>;
    using BucketBoundaries = std::vector<double>;

    PromImpl() = delete;
    PromImpl(const char* szHostport) : m_exposer(szHostport) {
      m_registry = std::make_shared<Registry>();
      m_exposer.RegisterCollectable(m_registry);

      DR_LOG(log_info) << "listening for prometheus scrape on " << szHostport;
    }
    ~PromImpl() {}

    void buildCounter(const string& name, const char* desc) {
      auto& m = BuildCounter()
        .Name(name)
        .Help(desc)
        .Register(*m_registry);

      std::shared_ptr<Family<Counter> > p(&m);
      m_mapCounter.insert(mapCounter_t::value_type(name, p));
    }

    void counterIncrement(const string& name, mapLabels_t& labels) {
      DR_LOG(log_info) << "counterIncrement";
      mapCounter_t::const_iterator it = m_mapCounter.find(name) ;
      if (m_mapCounter.end() != it) {
        DR_LOG(log_info) << "counterIncrement 2";
        it->second->Add(labels).Increment() ;
      }
    }
    void counterIncrement(const string& name, const double val, mapLabels_t& labels) {
      mapCounter_t::const_iterator it = m_mapCounter.find(name) ;
      if (m_mapCounter.end() != it) {
        it->second->Add(labels).Increment(val) ;
      }
    }

    void buildGauge(const string& name, const char* desc) {
      auto& m = BuildGauge()
        .Name(name)
        .Help(desc)
        .Register(*m_registry);

      std::shared_ptr<Family<Gauge> > p(&m);
      m_mapGauge.insert(mapGauge_t::value_type(name, p));
    }

    void gaugeIncrement(const string& name, mapLabels_t& labels) {
      mapGauge_t::const_iterator it = m_mapGauge.find(name) ;
      if (m_mapGauge.end() != it) {
        it->second->Add(labels).Increment() ;
      }
    }

    void gaugeIncrement(const string& name, const double val, mapLabels_t& labels) {
      mapGauge_t::const_iterator it = m_mapGauge.find(name) ;
      if (m_mapGauge.end() != it) {
        it->second->Add(labels).Increment(val) ;
      }
    }

    void gaugeDecrement(const string& name, mapLabels_t& labels) {
       mapGauge_t::const_iterator it = m_mapGauge.find(name) ;
      if (m_mapGauge.end() != it) {
        it->second->Add(labels).Decrement() ;
      }
    }

    void gaugeDecrement(const string& name, const double val, mapLabels_t& labels) {
       mapGauge_t::const_iterator it = m_mapGauge.find(name) ;
      if (m_mapGauge.end() != it) {
        it->second->Add(labels).Decrement(val) ;
      }
    }

    void gaugeSet(const string& name, const double val, mapLabels_t& labels) {
       mapGauge_t::const_iterator it = m_mapGauge.find(name) ;
      if (m_mapGauge.end() != it) {
        it->second->Add(labels).Set(val) ;
      }
    }

    void gaugeSetToCurrentTime(const string& name, mapLabels_t& labels) {
       mapGauge_t::const_iterator it = m_mapGauge.find(name) ;
      if (m_mapGauge.end() != it) {
        it->second->Add(labels).SetToCurrentTime() ;
      }
    }

  
  private:

    Exposer m_exposer;
    std::shared_ptr<Registry> m_registry;

    mapCounter_t  m_mapCounter;
    mapGauge_t  m_mapGauge;
  };

  StatsCollector::StatsCollector() : m_pimpl(nullptr) {

  }

  StatsCollector::~StatsCollector() {

  }

  void StatsCollector::enablePrometheus(const char* szHostport) {
    assert(nullptr == m_pimpl);
    m_pimpl = new PromImpl(szHostport);
  }

  // counters
  void StatsCollector::counterCreate(const string& name, const char* desc) {
    if (nullptr != m_pimpl) m_pimpl->buildCounter(name, desc);    
  }
  void StatsCollector::counterIncrement(const string& name, mapLabels_t labels) {
    if (nullptr != m_pimpl) m_pimpl->counterIncrement(name, labels); 
  }
  void StatsCollector::counterIncrement(const string& name, const double val, mapLabels_t labels) {
    if (nullptr != m_pimpl) m_pimpl->counterIncrement(name, val, labels); 
  }

  // gauges
  void StatsCollector::gaugeCreate(const string& name, const char* desc) {
    if (nullptr != m_pimpl) m_pimpl->buildGauge(name, desc);    
  }
  void StatsCollector::gaugeIncrement(const string& name, mapLabels_t labels) {
    if (nullptr != m_pimpl) m_pimpl->gaugeIncrement(name, labels); 
  }
  void StatsCollector::gaugeIncrement(const string& name, const double val, mapLabels_t labels) {
    if (nullptr != m_pimpl) m_pimpl->gaugeIncrement(name, val, labels); 
  }
  void StatsCollector::gaugeDecrement(const string& name, mapLabels_t labels) {
    if (nullptr != m_pimpl) m_pimpl->gaugeDecrement(name, labels); 
  }
  void StatsCollector::gaugeDecrement(const string& name, const double val, mapLabels_t labels) {
    if (nullptr != m_pimpl) m_pimpl->gaugeDecrement(name, val, labels); 
  }
  void StatsCollector::gaugeSet(const string& name, const double val, mapLabels_t labels) {
    if (nullptr != m_pimpl) m_pimpl->gaugeSet(name, val, labels); 
  }
  void StatsCollector::gaugeSetToCurrentTime(const string& name, mapLabels_t labels) {
    if (nullptr != m_pimpl) m_pimpl->gaugeSetToCurrentTime(name, labels); 
  }
}
