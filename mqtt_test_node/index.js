var mqtt = require('mqtt')
var client = mqtt.connect('mqtt://test.mosquitto.org:1883')

console.log('Connecting')

client.on('connect', function () {
  client.subscribe('morse/791548')
  console.log("Connected")
  client.publish('morse/791548', 'hinni,H');
})

client.on('message', function (topic, message) {
  // message is Buffer
  console.log(topic)
  console.log(message.toString())
  //client.end()
})