/* Program to log external IP-changes on NAT networks
 *
 * I have an ISP from Hell, and one problem I have noticed recently is that
 * my external IP address change a lot. To monitor this problem, I wrote this little
 * example program who checks the external IP every 5 minutes, and log changes.
 *
 * The program can be run in the background:
 *
 *      logip >> /var/tmp/ip.log 2>&1 &
 */


#include <ctime>
#include "restc-cpp/logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/SerializeJson.h"

using namespace restc_cpp;
using namespace std;

// Data structure returned from api.ipify.org
struct Data {
    string ip;
};

BOOST_FUSION_ADAPT_STRUCT(
    Data,
    (string, ip)
)

string now() {
    char date[32] = {};
    auto now = time(NULL);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M", localtime(&now));
    return date;
}

int main(int argc, char *argv[]) {

    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::info
    );

    const string url = "https://api.ipify.org";

    auto client = RestClient::Create();
    client->Process([&](Context& ctx) {

        string current_ip;
        Data data;

        while(true) {
            bool valid = false;
            try {
                SerializeFromJson(data, RequestBuilder(ctx)
                    .Get(url)
                    .Argument("format", "json")
                    .Header("X-Client", "RESTC_CPP")
                    .Execute());
                valid = true;
            } catch (const boost::exception& ex) {
                clog << now()
                    << "Caught boost exception: "
                    << boost::diagnostic_information(ex)
                    << endl;
            } catch (const exception& ex) {
                clog << now()
                    << "Caught exception: "
                    << ex.what()
                    << endl;
            }

            if (valid && (current_ip != data.ip)) {
                clog << now()
                    << ' '
                    << data.ip
                    << endl;
                current_ip = data.ip;
            }

            std::this_thread::sleep_for(std::chrono::minutes(5));
        }
    });


    return 0;
}
