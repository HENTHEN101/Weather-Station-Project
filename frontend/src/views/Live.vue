<template>
      <v-btn @click="openDialog" class="change-topic-btn" color="primary">Change Topic</v-btn>
  <v-container class="container" align="center">
      <v-card
        style="max-width: 1200px;margin-bottom: 20px; " 
      >
        <v-card-title>MQTT Topic & Server Info</v-card-title>
        <v-card-text>
          <div >Topic: {{ mqttTopic }}</div>
          <div >Server: {{ mqtthost }}</div>
        </v-card-text>
      </v-card>

    <v-row class="row">
      <v-col cols="9">
        <figure class="highcharts-figure" >
          <div id="container"></div>
        </figure>
      </v-col>
      <v-col cols="3">
        <v-card
          class="mb-5"
          style="max-width: 500px"
          color="#ee6b6e"
          subtitle="Temperature and Heat Index"
        >
          <v-card-item>
            <span class="text-onPrimaryContainer text-h3">
              {{ temperature }} / {{ heatindex }}
            </span>
          </v-card-item>
          <v-card-item>
            <v-select
              v-model="tempindexUnit"
              :items="['°C', '°F', 'K']"
              label="Unit"
            ></v-select>
          </v-card-item>
        </v-card>
      </v-col>
    </v-row>
    <v-row class="row" justify="start">
      <v-col cols="9">
        <figure class="highcharts-figure" >
          <div id="container1"></div>
        </figure>
      </v-col>
      <v-col cols="3">
        <v-card
          class="mb-5"
          style="max-width: 500px;"
          color="#29C5F6"
          subtitle="Humidity and Soil Moisture"
         >
          <v-card-item>
            <span class="text-onSecondaryContainer text-h3">
              {{ humidity }} / {{ soilmoisture }}
            </span>
          </v-card-item>
        </v-card>
      </v-col>
    </v-row>
    <v-row class="row" justify="start">
      <v-col cols="9">
        <figure class="highcharts-figure" >
          <div id="container2"></div>
        </figure>
      </v-col>
      <v-col cols="3">
        <v-card
          class="mb-5"
          style="max-width: 500px ;"
          color="#0B6E4F"
          subtitle="Pressure"
        >
        <v-card-item>
            <span class="text-onPrimaryContainer text-h3">
              {{ pressure }}
            </span>
          </v-card-item>
          <v-card-item>
            <v-select
              v-model="pressUnit"
              :items="['Pa', 'atm', 'mmHg']"
              label="Unit"
            ></v-select>
          </v-card-item>
        </v-card>
      </v-col>
    </v-row>
    <v-row class="row" justify="start">
      <v-col cols="9">
        <figure class="highcharts-figure" >
          <div id="container3"></div>
        </figure>
      </v-col>
      <v-col cols="3">
        <v-card
          class="mb-5"
          style="max-width: 500px;"
          color="#0BA045"
          subtitle="Altitude"
        >
        <v-card-item>
            <span class="text-onPrimaryContainer text-h3" >
              {{ altitude }}
            </span>
          </v-card-item>
          <v-card-item>
            <v-select
              v-model="altUnit"
              :items="['m', 'km', 'miles']"
              label="Unit"
            ></v-select>
          </v-card-item>
        </v-card>
      </v-col>
    </v-row>
  </v-container>

  <!-- Dialog for MQTT Topic Input -->
  <v-dialog v-model="dialog" max-width="600px">
    <v-card>
      <v-card-title>Enter MQTT Topic</v-card-title>
      <v-card-text>
        <v-text-field v-model="mqttTopic" label="Topic" outlined ></v-text-field>
      </v-card-text>
      <v-card-actions>
        <v-btn color="primary" @click="subscribeToTopic">Subscribe</v-btn>
        <v-btn color="primary" @click="Cancel">Cancel</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>

  <v-dialog v-model="dialog2" max-width="400px">
    <v-card>
      <v-card-item>
        <div>{{ type }} Connection {{ checkstat }}. {{ refresh }}</div>
      </v-card-item>
    </v-card>
  </v-dialog>
</template>

<script setup>
/** JAVASCRIPT HERE */

// IMPORTS

import { useMqttStore } from "@/store/mqttStore"; // Import Mqtt Store
import { storeToRefs } from "pinia";

// Highcharts, Load the exporting module and Initialize exporting module.
import Highcharts from "highcharts";
import more from "highcharts/highcharts-more";
import Exporting from "highcharts/modules/exporting";
Exporting(Highcharts);
more(Highcharts);

import {
  ref,
  reactive,
  watch,
  onMounted,
  onBeforeUnmount,
  computed,
} from "vue";

import { useRoute, useRouter } from "vue-router";

// VARIABLES
const router = useRouter();
const route = useRoute();

const Mqtt = useMqttStore();
const { payload, payloadTopic } = storeToRefs(Mqtt);
const mqttTopic = ref(""); // Variable to store MQTT topic input
const mqtthost = ref("www.yanacreations.com");
const checkstat = ref("failed");
const type = ref("Server");
const refresh = ref("Page will refresh in 5s");
const dialog = ref(false); // Variable to control the dialog visibility
const dialog2=ref(false);

const tempHiChart = ref(null); // Chart object
const humChart = ref(null); // Chart object
const pressChart = ref(null); // Chart object
const altChart = ref(null); // Chart object

const tempindexUnit =ref("°C");
const pressUnit = ref("Pa");
const altUnit = ref("m");

const points = ref(600); // Specify the quantity of points to be shown on the live graph simultaneously.
const shift = ref(false); // Delete a point from the left side and append a new point to the right side of the graph.

// FUNCTIONS

onMounted(() => {
  // THIS FUNCTION IS CALLED AFTER THIS COMPONENT HAS BEEN MOUNTED
  //autoRefresh();

  /*const storedTopic = localStorage.getItem("mqttTopic");
  if(storedTopic !==""){
    mqttTopic.value = storedTopic;
  }*/
  CreateCharts();
  CreateCharts_2();
  CreateCharts_3();
  CreateCharts_4();

  Mqtt.connect(); // Connect to Broker located on the backend
  dialog.value=true;

 
});
const openDialog = () => {
  dialog.value = true;
};

const Cancel = () =>{
  dialog.value=false;
};

const check1 = () => {
  dialog2.value=true;
};
const check2 =()=> {
  refresh.value = "";
  checkstat.value  = "success";
  dialog2.value=true;
};
const check3 =()=> {
  type.value = "Topic";
  dialog2.value=true;
};
const check4 =()=> {
  refresh.value = "";
  type.value = "Topic";
  checkstat.value  = "success";
  dialog2.value=true;
};
const check5 =()=> {
  type.value = "Connection";
  checkstat.value  = "lost";
  dialog2.value=true;
};

const subscribeToTopic = () => {
 // Unsubscribe from the previous topic
  //Mqtt.unsubcribe(mqttTopic.value);
  // Subscribe to the MQTT topic provided by the user
  if (mqttTopic.value.trim() !== "" ) {
    clearCharts();
    Mqtt.subscribe(mqttTopic.value);
    Mqtt.subscribe(mqttTopic.value + "_sub");
    dialog.value = false; // Close the dialog after subscription
   localStorage.setItem("mqttTopic", mqttTopic.value); // Store the topic in local storage
  } else {
    // Show an error message if the topic is empty
    alert("Please enter a valid MQTT topic.");
  }
};

onBeforeUnmount(() => {
  // THIS FUNCTION IS CALLED RIGHT BEFORE THIS COMPONENT IS UNMOUNTED
  // unsubscribe from all topics
  Mqtt.unsubcribeAll();
});

const CreateCharts = async () => {
  // TEMPERATURE CHART
  tempHiChart.value = Highcharts.chart("container", {
    chart: { zoomType: "x" },
    title: { text: "Air Temperature and Heat Index Analysis(Live)", align: "left" },
    yAxis: {
      title: {
        text: "°C",
        style: { color: "#000000" },
      },
      labels: { format: "{value} °C" },
    },
    xAxis: {
      type: "datetime",
      title: { text: "Time", style: { color: "#000000" } },
    },
    tooltip: { shared: true },
    series: [
      {
        name: "Temperature",
        type: "spline",
        data: [],
        turboThreshold: 0,
        color: Highcharts.getOptions().colors[0],
      },
      {
        name: "Heat Index",
        type: "spline",
        data: [],
        turboThreshold: 0,
        color: Highcharts.getOptions().colors[1],
      },
    ],
  });
};

const CreateCharts_2 = async () => {
  // Humidity CHART
  humChart.value = Highcharts.chart("container1", {
    chart: { zoomType: "x" },
    title: { text: "Humidity and Soil Moisture Analysis(Live)", align: "left" },
    yAxis: {
      title: {
        text: "%",
        style: { color: "#000000" },
      },
      labels: { format: "{value} %" },
    },
    xAxis: {
      type: "datetime",
      title: { text: "Time", style: { color: "#000000" } },
    },
    tooltip: { shared: true },
    series: [
      {
        name: "Humidity",
        type: "spline",
        data: [],
        turboThreshold: 0,
        color: Highcharts.getOptions().colors[0],
      },
      {
        name: "Soil Moisture",
        type: "spline",
        data: [],
        turboThreshold: 0,
        color: Highcharts.getOptions().colors[3],
      },
    ],
  });
};

const CreateCharts_3 = async () => {
  // Humidity CHART
  pressChart.value = Highcharts.chart("container2", {
    chart: { zoomType: "x" },
    title: { text: "Pressure Analysis(Live)", align: "left" },
    yAxis: {
      title: {
        text: "Pa",
        style: { color: "#000000" },
      },
      labels: { format: "{value} Pa" },
    },
    xAxis: {
      type: "datetime",
      title: { text: "Time", style: { color: "#000000" } },
    },
    tooltip: { shared: true },
    series: [
      {
        name: "Pressure",
        type: "spline",
        data: [],
        turboThreshold: 0,
        color: Highcharts.getOptions().colors[3],
      },
    ],
  });
};

const CreateCharts_4 = async () => {
  // Humidity CHART
  altChart.value = Highcharts.chart("container3", {
    chart: { zoomType: "x" },
    title: { text: "Altitude Analysis(Live)", align: "left" },
    yAxis: {
      title: {
        text: "m",
        style: { color: "#000000" },
      },
      labels: { format: "{value} m" },
    },
    xAxis: {
      type: "datetime",
      title: { text: "Time", style: { color: "#000000" } },
    },
    tooltip: { shared: true },
    series: [
      {
        name: "Altitude",
        type: "spline",
        data: [],
        turboThreshold: 0,
        color: Highcharts.getOptions().colors[4],
      },
    ],
  });
};

// COMPUTED PROPERTIES
const temperature = computed(() => {
  if (!!payload.value) {
    let temp = payload.value.temperature;
    if (tempindexUnit.value === "°F") {
      temp = (temp * 9) / 5 + 32;
    } else if (tempindexUnit.value === "K") {
      temp += 273.15;
    }
    return `${temp.toFixed(2)} ${tempindexUnit.value}` ;
  }
});
const heatindex = computed(() => {
  if (!!payload.value) {
    let heatindex = payload.value.heatindex;
    if (tempindexUnit.value === "°F") {
      heatindex = (heatindex * 9) / 5 + 32;
    } else if (tempindexUnit.value === "K") {
      heatindex += 273.15;
    }
    return `${heatindex.toFixed(2)} ${tempindexUnit.value}`;
  }
});
const humidity = computed(() => {
  if (!!payload.value) {
    return `${payload.value.humidity.toFixed(2)} %`;
  }
});
const soilmoisture = computed(() => {
  if (!!payload.value) {
    return `${payload.value.soilmoisture.toFixed(2)} %`;
  }
});
const pressure = computed(() => {
  if (!!payload.value) {
    let press = payload.value.pressure;
    if(pressUnit.value === "atm"){
      press = (press/101300);
    }else if(pressUnit.value === "mmHg"){
      press = (press/133.3);
    }
    return `${press.toFixed(2)} ${pressUnit.value}`;
  }
});
const altitude = computed(() => {
  if (!!payload.value) {
    let alt = payload.value.altitude;
    if(altUnit.value === "km"){
      alt = (alt/1000);
    }else if(altUnit.value === "miles"){
      alt = (alt/1609);
    }
    return `${alt.toFixed(2)} ${altUnit.value}`;
  }
});


// WATCHERS
watch(payload, (data) => {
  if (points.value > 0 ) {
    points.value--;
  } else {
    shift.value = true;
  }
  tempHiChart.value.series[0].addPoint(
    { y: parseFloat(data.temperature.toFixed(2)), x: data.timestamp * 1000 },
    true,
    shift.value
  );
  tempHiChart.value.series[1].addPoint(
    { y: parseFloat(data.heatindex.toFixed(2)), x: data.timestamp * 1000 },
    true,
    shift.value
  );
  humChart.value.series[0].addPoint(
    { y: parseFloat(data.humidity.toFixed(2)), x: data.timestamp * 1000 },
    true,
    shift.value
  );
  humChart.value.series[1].addPoint(
    { y: parseFloat(data.soilmoisture.toFixed(2)), x: data.timestamp * 1000 },
    true,
    shift.value
  );
  pressChart.value.series[0].addPoint(
    { y: parseFloat(data.pressure.toFixed(2)), x: data.timestamp * 1000 },
    true,
    shift.value
  );
  altChart.value.series[0].addPoint(
    { y: parseFloat(data.altitude.toFixed(2)), x: data.timestamp * 1000 },
    true,
    shift.value
  );
});

const autoRefresh = () => {
  setTimeout(() => {
    location.reload(); // Reload the page
  }, 5000); // Refresh every 5000 milliseconds (5 seconds)
};



const clearCharts = () => {
  // Clear chart data
  tempHiChart.value.series[0].setData([], true);
  tempHiChart.value.series[1].setData([], true);
  humChart.value.series[0].setData([], true);
  humChart.value.series[1].setData([], true);
  pressChart.value.series[0].setData([], true);
  altChart.value.series[0].setData([], true);
};

document.addEventListener('HostFailEvent',check1);
document.addEventListener('FailTopicEvent',check3);
document.addEventListener('LostEvent',check5);
document.addEventListener('LostEvent',autoRefresh);
document.addEventListener('HostFailEvent', autoRefresh);
document.addEventListener('FailTopicEvent', autoRefresh);
document.addEventListener('HostConnectEvent',check2);
document.addEventListener('ConnectTopicEvent',check4);
</script>


<style scoped>
.row {
  max-width: 1200px;
}
.container {
  background: linear-gradient(to bottom, #ee6b6e, #29C5F6,#0B6E4F, #0BA045);
  width: 1200px;
  height: 1900px;
  margin-right:100px ;
  margin-top: -50px;
}
figure {
  border: 2px solid rgb(0, 0, 0);
}

</style>