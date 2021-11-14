#include "Utils.h"
#include "automotive.h"
#include "automotiveSupport.h"
#include "ndds/ndds_cpp.h"


static int shutdown(
    DDSDomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = participant->delete_contained_entities();
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDSTheParticipantFactory->delete_participant(participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    return status;
}

extern "C" int publisher_main(int sample_count)
{
    DDSDomainParticipant *participant = NULL;
    DDSPublisher *publisher = NULL;
    DDSTopic *topic = NULL;
    DDSDataWriter *writer = NULL;

    Helloworld_HelloWordMessageDataWriter* Hello_WorldMessage_writer = NULL;
    Helloworld_HelloWordMessage* helloworld_instance = NULL;
    
    DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
    int domainId = 0;
    const char *type_name = NULL;
    DDS_ReturnCode_t retcode;
    DDS_Duration_t send_period = {4,0};


    PropertyUtil* prop = new PropertyUtil("hello_world.properties");
    domainId = prop -> getLongProperty("config.domainId");
    
    long time = prop->getLongProperty("config.pubInterval");
    send_period.sec = time / 1000;
    send_period.nanosec = (time % 1000) * 1000 * 1000;

    std::string platformTopicName = prop->getStringProperty("topic.Hello");
    if (platformTopicName == "") {
        printf("No platform topic name specified\n");
        return -1;
    }
    //Qos Parametrelerinin Olduğu Dosya Adi Alinir.
    std::string qosLibrary = prop->getStringProperty("qos.Library");
    if (qosLibrary == "") {
        printf("No QoS Library specified\n");
        return -1;
    }

    //İlgili Component İçin Gerekli kütüphane detaylari alinir.
    std::string HelloWorldQosProfile = prop->getStringProperty("qos.Hello.Profile");
    if (HelloWorldQosProfile == "") {
        printf("No Planning QoS Profile specified\n");
        return -1;
    }

    //Participant oluşturuldu.
    participant = DDSTheParticipantFactory->create_participant_with_profile(
        domainId, qosLibrary.c_str(), HelloWorldQosProfile.c_str(),
        NULL /* listener */, DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        shutdown(participant);
        return -1;
    }

    //Publisher oluşturuldu
    publisher = participant->create_publisher_with_profile(
        qosLibrary.c_str(), HelloWorldQosProfile.c_str(), NULL /* listener */, DDS_STATUS_MASK_NONE);
    if (publisher == NULL) {
        printf("create_publisher error\n");
        shutdown(participant);
        return -1;
    }

    type_name = Helloworld_HelloWordMessageTypeSupport::get_type_name();
    retcode = Helloworld_HelloWordMessageTypeSupport::register_type(
        participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        printf("register_type error %d\n", retcode);
        shutdown(participant);
        return -1;
    }

    topic = participant->create_topic_with_profile(
        platformTopicName.c_str(),
        type_name, qosLibrary.c_str(), HelloWorldQosProfile.c_str(), NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        printf("create_topic error\n");
        shutdown(participant);
        return -1;
    }

    writer = publisher->create_datawriter_with_profile(
        topic, qosLibrary.c_str(), HelloWorldQosProfile.c_str(), NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (writer == NULL) {
        printf("create_datawriter error\n");
        shutdown(participant);
        return -1;
    }

    Hello_WorldMessage_writer = Helloworld_HelloWordMessageDataWriter::narrow(writer);
    if (Hello_WorldMessage_writer == NULL) {
        printf("DataWriter narrow error\n");
        shutdown(participant);
        return -1;
    }

    helloworld_instance = Helloworld_HelloWordMessageTypeSupport::create_data();
    if (helloworld_instance == NULL) {
        printf("Platform_PlatformStatusTypeSupport::create_data error\n");
        shutdown(participant);
        return -1;
    }

    for (int count=0; (sample_count == 0) || (count < sample_count); ++count) {
        
        printf("\r\n Hello World Message Sended");
        
        if (count % 2 == 0) {
            helloworld_instance->msg = (char*)"Test";
        }

        else {
            helloworld_instance->msg = (char*)"Hello World";
        }
      
        retcode = Hello_WorldMessage_writer->write(*helloworld_instance, instance_handle);
        if (retcode != DDS_RETCODE_OK) {
            printf("write error %d\n", retcode);
        }

        NDDSUtility::sleep(send_period);
    }

    retcode = Helloworld_HelloWordMessageTypeSupport::delete_data(helloworld_instance);
    if (retcode != DDS_RETCODE_OK) {
        printf("Helloworld_HelloWordMessageTypeSupport::delete_data error %d\n", retcode);
    }

    /* Delete all entities */
    return shutdown(participant);
}

int main(int argc, char *argv[])
{
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        sample_count = atoi(argv[1]);
    }

    printf("\r\n Hello World Window");

    /* Uncomment this to turn on additional logging
    NDDSConfigLogger::get_instance()->
    set_verbosity_by_category(NDDS_CONFIG_LOG_CATEGORY_API, 
    NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */

    return publisher_main(sample_count);
}