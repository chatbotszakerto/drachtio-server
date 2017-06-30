/*
Copyright (c) 2013, David C Horton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <iostream>
#include <fstream>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include "drachtio-config.hpp"

#include "controller.hpp"

using namespace std ;
using boost::property_tree::ptree;
using boost::property_tree::ptree_error;

namespace drachtio {

     class DrachtioConfig::Impl {
    public:
        Impl( const char* szFilename, bool isDaemonized) : m_bIsValid(false), m_adminPort(0), m_bDaemon(isDaemonized), m_bConsoleLogger(false) {

            // default timers
            m_nTimerT1 = 500 ;
            m_nTimerT2 = 4000 ;
            m_nTimerT4 = 5000 ;
            m_nTimerT1x64 = 32000 ;

            try {
                std::filebuf fb;
                if( !fb.open (szFilename,std::ios::in) ) {
                    cerr << "Unable to open configuration file: " << szFilename << endl ;
                    return ;
                }
                std::istream is(&fb);
                
                ptree pt;
                read_xml(is, pt);
                
                /* admin configuration */
                try {
                    pt.get_child("drachtio.admin") ; // will throw if doesn't exist
                    m_adminPort = pt.get<unsigned int>("drachtio.admin.<xmlattr>.port", 9022) ;
                    m_secret = pt.get<string>("drachtio.admin.<xmlattr>.secret", "admin") ;
                    m_adminAddress = pt.get<string>("drachtio.admin") ;
                } catch( boost::property_tree::ptree_bad_path& e ) {
                    cerr << "XML tag <admin> not found; this is required to provide admin socket details" << endl ;
                    return ;
                }


                /* sip contacts */

                // old way: a single contact
                try {
                    string strUrl = pt.get<string>("drachtio.sip.contact") ;
                    m_vecSipUrl.push_back( make_pair(strUrl,"") ) ;
                    m_vecTransports.push_back( boost::make_shared<SipTransport>(strUrl) );

                } catch( boost::property_tree::ptree_bad_path& e ) {

                    //good, hopefull they moved to the new way: a parent <contacts> tag containing multiple contacts
                    try {
                         BOOST_FOREACH(ptree::value_type &v, pt.get_child("drachtio.sip.contacts")) {
                            // v.first is the name of the child.
                            // v.second is the child tree.
                            if( 0 == v.first.compare("contact") ) {
                                string external = v.second.get<string>("<xmlattr>.external-ip","") ;            
                                string localNet = v.second.get<string>("<xmlattr>.local-net","") ;            
                                m_vecSipUrl.push_back( make_pair(v.second.data(), external) );
                                m_vecTransports.push_back( boost::make_shared<SipTransport>(v.second.data(), localNet, external) );
                            }
                        }
                    } catch( boost::property_tree::ptree_bad_path& e ) {
                        //neither <contact> nor <contacts> found: presumably will be provided on command line
                    }
                }

                m_sipOutboundProxy = pt.get<string>("drachtio.sip.outbound-proxy", "") ;

                m_tlsKeyFile = pt.get<string>("drachtio.sip.tls.key-file", "") ;
                m_tlsCertFile = pt.get<string>("drachtio.sip.tls.cert-file", "") ;
                m_tlsChainFile = pt.get<string>("drachtio.sip.tls.chain-file", "") ;

                
                /* logging configuration  */
 
                m_nSofiaLogLevel = pt.get<unsigned int>("drachtio.logging.sofia-loglevel", 1) ;

                // syslog
                try {
                    m_syslogAddress = pt.get<string>("drachtio.logging.syslog.address") ;
                    m_sysLogPort = pt.get<unsigned int>("drachtio.logging.syslog.port", 0) ;
                    m_syslogFacility = pt.get<string>("drachtio.logging.syslog.facility") ;
                } catch( boost::property_tree::ptree_bad_path& e ) {
                }

                // file log
                try {
                    m_logFileName = pt.get<string>("drachtio.logging.file.name") ;
                    m_logArchiveDirectory = pt.get<string>("drachtio.logging.file.archive", "archive") ;
                    m_rotationSize = pt.get<unsigned int>("drachtio.logging.file.size", 5) ;
                    m_maxSize = pt.get<unsigned int>("drachtio.logging.file.maxSize", 16 * 1000 * 1000) ; //max size of stored files: 16M default
                    m_minSize = pt.get<unsigned int>("drachtio.logging.file.c", 2 * 1000 * 1000 * 1000) ;//min free space on disk: 2G default
                    m_bAutoFlush = pt.get<bool>("drachtio.logging.file.auto-flush", false) ;
                } catch( boost::property_tree::ptree_bad_path& e ) {
                }

                if( (0 == m_logFileName.length() && 0 == m_syslogAddress.length()) || pt.get_child_optional("drachtio.logging.console") ) {
                    m_bConsoleLogger = true ;
                }
                string loglevel = pt.get<string>("drachtio.logging.loglevel", "info") ;
                
                if( 0 == loglevel.compare("notice") ) m_loglevel = log_notice ;
                else if( 0 == loglevel.compare("error") ) m_loglevel = log_error ;
                else if( 0 == loglevel.compare("warning") ) m_loglevel = log_warning ;
                else if( 0 == loglevel.compare("info") ) m_loglevel = log_info ;
                else if( 0 == loglevel.compare("debug") ) m_loglevel = log_debug ;
                else m_loglevel = log_info ;                    

                // timers
                try {
                        m_nTimerT1 = pt.get<unsigned int>("drachtio.sip.timers.t1", 500) ; 
                        m_nTimerT2 = pt.get<unsigned int>("drachtio.sip.timers.t2", 4000) ; 
                        m_nTimerT4 = pt.get<unsigned int>("drachtio.sip.timers.t4", 5000) ; 
                        m_nTimerT1x64 = pt.get<unsigned int>("drachtio.sip.timers.t1x64", 32000) ; 

                } catch( boost::property_tree::ptree_bad_path& e ) {
                    if( !m_bDaemon ) {
                        cout << "invalid timer configuration" << endl ;
                    }
                }

                // spammers
                try {
                    BOOST_FOREACH(ptree::value_type &v, pt.get_child("drachtio.sip.spammers")) {
                        // v.first is the name of the child.
                        // v.second is the child tree.
                        if( 0 == v.first.compare("header") ) {
                            ptree pt = v.second ;

                            string header = pt.get<string>("<xmlattr>.name", ""); 
                            if( header.length() > 0 ) {
                                std::vector<string> vec ;
                                BOOST_FOREACH(ptree::value_type &v, pt ) {
                                    if( v.second.data().length() > 0 ) {
                                        vec.push_back( v.second.data() ) ;
                                    }
                                }
                                std::transform(header.begin(), header.end(), header.begin(), ::tolower) ;
                                m_mapSpammers.insert( make_pair( header,vec ) ) ;
                            }
                        }
                        m_actionSpammer = pt.get<string>("drachtio.sip.spammers.<xmlattr>.action", "discard") ;
                        m_tcpActionSpammer = pt.get<string>("drachtio.sip.spammers.<xmlattr>.tcp-action", "discard") ;
                    }
                } catch( boost::property_tree::ptree_bad_path& e ) {
                    //no spammer config...its optional
                }

                string cdrs = pt.get<string>("drachtio.cdrs", "") ;
                transform(cdrs.begin(), cdrs.end(), cdrs.begin(), ::tolower);
                m_bGenerateCdrs = ( 0 == cdrs.compare("true") || 0 == cdrs.compare("yes") ) ;
                
                fb.close() ;
                                               
                m_bIsValid = true ;
            } catch( exception& e ) {
                cerr << "Error reading configuration file: " << e.what() << endl ;
            }    
        }
        ~Impl() {
        }
                   
        bool isValid() const { return m_bIsValid; }
        const string& getSyslogAddress() const { return m_syslogAddress; }
        unsigned int getSyslogPort() const { return m_sysLogPort ; }        
        bool getSyslogTarget( std::string& address, unsigned int& port ) const {
            if( m_syslogAddress.length() > 0 ) {
                address = m_syslogAddress ;
                port = m_sysLogPort  ;
                return true ;
            }
            return false ;
        }
        bool getFileLogTarget( std::string& fileName, std::string& archiveDirectory, unsigned int& rotationSize, bool& autoFlush, 
            unsigned int& maxSize, unsigned int& minSize ) {
            if( m_logFileName.length() > 0 ) {
                fileName = m_logFileName ;
                archiveDirectory = m_logArchiveDirectory ;
                rotationSize = m_rotationSize ;
                autoFlush = m_bAutoFlush;
                return true ;
            }
            return false ;
        }
        bool getConsoleLogTarget() {
            return m_bConsoleLogger ;
        }

		severity_levels getLoglevel() {
			return m_loglevel ;
		}
        unsigned int getSofiaLogLevel(void) { return m_nSofiaLogLevel; }
      
        bool getSyslogFacility( sinks::syslog::facility& facility ) const {
        
            if( m_syslogFacility.empty() ) return false ;
            
            if( m_syslogFacility == "local0" ) facility = sinks::syslog::local0 ;
            else if( m_syslogFacility == "local1" ) facility = sinks::syslog::local1 ;
            else if( m_syslogFacility == "local2" ) facility = sinks::syslog::local2 ;
            else if( m_syslogFacility == "local3" ) facility = sinks::syslog::local3 ;
            else if( m_syslogFacility == "local4" ) facility = sinks::syslog::local4 ;
            else if( m_syslogFacility == "local5" ) facility = sinks::syslog::local5 ;
            else if( m_syslogFacility == "local6" ) facility = sinks::syslog::local6 ;
            else if( m_syslogFacility == "local7" ) facility = sinks::syslog::local7 ;
            else return false ;
            
            return true ;
        }
        void getSipUrls( vector< pair<string,string> >& urls) const { urls = m_vecSipUrl; }

        bool getSipOutboundProxy( string& sipOutboundProxy ) const {
            sipOutboundProxy = m_sipOutboundProxy ;
            return sipOutboundProxy.length() > 0 ;
        }

        bool getTlsFiles( string& tlsKeyFile, string& tlsCertFile, string& tlsChainFile ) {
            tlsKeyFile = m_tlsKeyFile ;
            tlsCertFile = m_tlsCertFile ;
            tlsChainFile = m_tlsChainFile ;

            // both key and cert are minimally required
            return tlsKeyFile.length() > 0 && tlsCertFile.length() > 0 ;
        }
        
        unsigned int getAdminPort( string& address ) {
            address = m_adminAddress ;
            return m_adminPort ;
        }
        bool isSecret( const string& secret ) {
            return 0 == secret.compare( m_secret ) ;
        }
        bool generateCdrs(void) const {
            return m_bGenerateCdrs ;
        }

        void getTimers( unsigned int& t1, unsigned int& t2, unsigned int& t4, unsigned int& t1x64 ) {
            t1 = m_nTimerT1 ;
            t2 = m_nTimerT2 ;
            t4 = m_nTimerT4 ;
            t1x64 = m_nTimerT1x64 ;
        }

        DrachtioConfig::mapHeader2Values& getSpammers( string& action, string& tcpAction ) {
            if( !m_mapSpammers.empty() ) {
                action = m_actionSpammer ;
                tcpAction = m_tcpActionSpammer ;
            }
            return m_mapSpammers ;
        }

        void getTransports(vector< boost::shared_ptr<SipTransport> >& transports) const {
            transports = m_vecTransports ;
        }


 
    private:
        
        bool getXmlAttribute( ptree::value_type const& v, const string& attrName, string& value ) {
            try {
                string key = "<xmlattr>." ;
                key.append( attrName ) ;
                value = v.second.get<string>( key ) ;
            } catch( const ptree_error& err ) {
                return false ;
            }
            if( value.empty() ) return false ;
            return true ;
        }
    
        bool m_bIsValid ;
        vector< pair<string,string> > m_vecSipUrl ;
        string m_sipOutboundProxy ;
        string m_syslogAddress ;
        string m_logFileName ;
        string m_logArchiveDirectory ;
        string m_tlsKeyFile ;
        string m_tlsCertFile ;
        string m_tlsChainFile ;
        bool m_bAutoFlush ;
        unsigned int m_rotationSize ;
        unsigned int m_maxSize ;
        unsigned int m_minSize ;
        unsigned int m_sysLogPort ;
        string m_syslogFacility ;
        severity_levels m_loglevel ;
        unsigned int m_nSofiaLogLevel ;
        string m_adminAddress ;
        unsigned int m_adminPort ;
        string m_secret ;
        bool m_bGenerateCdrs ;
        bool m_bDaemon;
        bool m_bConsoleLogger ;
        unsigned int m_nTimerT1, m_nTimerT2, m_nTimerT4, m_nTimerT1x64 ;
        string m_actionSpammer ;
        string m_tcpActionSpammer ;
        mapHeader2Values m_mapSpammers ;
        vector< boost::shared_ptr<SipTransport> >  m_vecTransports;

  } ;
    
    /*
     Public interface
    */
    DrachtioConfig::DrachtioConfig( const char* szFilename, bool isDaemonized ) : m_pimpl( new Impl(szFilename, isDaemonized) ) {
    }
    
    DrachtioConfig::~DrachtioConfig() {
       delete m_pimpl ;
    }
    
    bool DrachtioConfig::isValid() {
        return m_pimpl->isValid() ;
    }
    unsigned int DrachtioConfig::getSofiaLogLevel() {
        return m_pimpl->getSofiaLogLevel() ;
    }
   
    bool DrachtioConfig::getSyslogTarget( std::string& address, unsigned int& port ) const {
        return m_pimpl->getSyslogTarget( address, port ) ;
    }
    bool DrachtioConfig::getSyslogFacility( sinks::syslog::facility& facility ) const {
        return m_pimpl->getSyslogFacility( facility ) ;
    }
    bool DrachtioConfig::getFileLogTarget( std::string& fileName, std::string& archiveDirectory, unsigned int& rotationSize, 
        bool& autoFlush, unsigned int& maxSize, unsigned int& minSize ) {
        return m_pimpl->getFileLogTarget( fileName, archiveDirectory, rotationSize, autoFlush, maxSize, minSize ) ;
    }
    bool DrachtioConfig::getConsoleLogTarget() {
        return m_pimpl->getConsoleLogTarget() ;
    }

    void DrachtioConfig::getSipUrls( std::vector< pair<string,string> >& urls ) const {
        m_pimpl->getSipUrls(urls) ;
    }
    bool DrachtioConfig::getSipOutboundProxy( std::string& sipOutboundProxy ) const {
        return m_pimpl->getSipOutboundProxy( sipOutboundProxy ) ;
    }

    void DrachtioConfig::Log() const {
        DR_LOG(log_notice) << "Configuration:" << endl ;
    }
    severity_levels DrachtioConfig::getLoglevel() {
        return m_pimpl->getLoglevel() ;
    }
    unsigned int DrachtioConfig::getAdminPort( string& address ) {
        return m_pimpl->getAdminPort( address ) ;
    }
    bool DrachtioConfig::isSecret( const string& secret ) const {
        return m_pimpl->isSecret( secret ) ;
    }
    bool DrachtioConfig::getTlsFiles( std::string& keyFile, std::string& certFile, std::string& chainFile ) const {
        return m_pimpl->getTlsFiles( keyFile, certFile, chainFile ) ;
    }
    bool DrachtioConfig::generateCdrs(void) const {
        return m_pimpl->generateCdrs() ;
    }
    void DrachtioConfig::getTimers( unsigned int& t1, unsigned int& t2, unsigned int& t4, unsigned int& t1x64 ) {
        return m_pimpl->getTimers( t1, t2, t4, t1x64 ) ;
    }
    DrachtioConfig::mapHeader2Values& DrachtioConfig::getSpammers( string& action, string& tcpAction ) {
        return m_pimpl->getSpammers( action, tcpAction ) ;
    }
    void DrachtioConfig::getTransports(vector< boost::shared_ptr<SipTransport> >& transports) const {
        return m_pimpl->getTransports(transports) ;
    }



}
