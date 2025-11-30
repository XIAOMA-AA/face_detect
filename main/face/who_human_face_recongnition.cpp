#include "who_human_face_recongnition.hpp"

void task_face_reconginition(void *pvParameters)
{
    FaceRecognition112V1S8 *recognizer = new FaceRecognition112V1S8();
    recognizer->set_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "fr");
    int partition_result = recognizer->set_ids_from_flash();

    while (1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void who_human_face_recongnition(void)
{
}
