# Prometheus.io integration

drachtio exposes the following metrics to Prometheus:
```
./drachtio --prometheus-scrape-port 9099 -f ../test/drachtio.conf.xml &

$ curl 127.0.0.1:9099/metrics
# HELP exposer_transferred_bytes_total Transferred bytes to metrics services
# TYPE exposer_transferred_bytes_total counter
exposer_transferred_bytes_total 0.000000
# HELP exposer_scrapes_total Number of times metrics were scraped
# TYPE exposer_scrapes_total counter
exposer_scrapes_total 0.000000
# HELP exposer_request_latencies Latencies of serving scrape requests, in microseconds
# TYPE exposer_request_latencies summary
exposer_request_latencies_count 0
exposer_request_latencies_sum 0.000000
exposer_request_latencies{quantile="0.500000"} Nan
exposer_request_latencies{quantile="0.900000"} Nan
exposer_request_latencies{quantile="0.990000"} Nan
# HELP drachtio_sip_requests_total count of sip requests
# TYPE drachtio_sip_requests_total counter
# HELP drachtio_sip_responses_total count of sip responses
# TYPE drachtio_sip_responses_total counter
# HELP drachtio_build_info drachtio version running
# TYPE drachtio_build_info counter
drachtio_build_info{version="v0.8.0-rc7-20-gaf3ddfac7"} 1.000000
# HELP drachtio_time_started drachtio start time
# TYPE drachtio_time_started gauge
drachtio_time_started 1555075955.000000
# HELP drachtio_stable_dialogs count of SIP dialogs in progress
# TYPE drachtio_stable_dialogs gauge
# HELP drachtio_proxy_cores count of proxied call setups in progress
# TYPE drachtio_proxy_cores gauge
# HELP drachtio_registered_endpoints count of registered endpoints
# TYPE drachtio_registered_endpoints gauge
# HELP drachtio_app_connections count of connections to drachtio applications
# TYPE drachtio_app_connections gauge
# HELP sofia_client_txn_hash_size current size of sofia hash table for client transactions
# TYPE sofia_client_txn_hash_size gauge
# HELP sofia_server_txn_hash_size current size of sofia hash table for server transactions
# TYPE sofia_server_txn_hash_size gauge
# HELP sofia_dialog_hash_size current size of sofia hash table for dialogs
# TYPE sofia_dialog_hash_size gauge
# HELP sofia_server_txns_total count of sofia server-side transactions
# TYPE sofia_server_txns_total gauge
# HELP sofia_client_txns_total count of sofia client-side transactions
# TYPE sofia_client_txns_total gauge
# HELP sofia_dialogs_total count of sofia dialogs
# TYPE sofia_dialogs_total gauge
# HELP sofia_msgs_recv_total count of sip messages received by sofia sip stack
# TYPE sofia_msgs_recv_total gauge
# HELP sofia_msgs_sent_total count of sip messages sent by sofia sip stack
# TYPE sofia_msgs_sent_total gauge
# HELP sofia_requests_recv_total count of sip requests received by sofia sip stack
# TYPE sofia_requests_recv_total gauge
# HELP sofia_requests_sent_total count of sip requests sent by sofia sip stack
# TYPE sofia_requests_sent_total gauge
# HELP sofia_bad_msgs_recv_total count of invalid sip messages received by sofia sip stack
# TYPE sofia_bad_msgs_recv_total gauge
# HELP sofia_bad_reqs_recv_total count of invalid sip requests received by sofia sip stack
# TYPE sofia_bad_reqs_recv_total gauge
# HELP sofia_retransmitted_requests_total count of sip requests retransmitted by sofia sip stack
# TYPE sofia_retransmitted_requests_total gauge
# HELP sofia_retransmitted_responses_total count of sip responses retransmitted by sofia sip stack
# TYPE sofia_retransmitted_responses_total gauge
# HELP drachtio_call_answer_seconds call answer seconds
# TYPE drachtio_call_answer_seconds histogram
# HELP drachtio_call_pdd_seconds call post-dial delay seconds
# TYPE drachtio_call_pdd_seconds histogram
MacBook-Pro-4:build dhorton$
```