var mqtt = require('mqtt')
var client = mqtt.connect('mqtt://mqtt.thingspeak.com:1883')

console.log('Connecting')

client.on('connect', function () {
  client.subscribe('channels/791548/subscribe/csv')
  console.log("Connected")
  client.publish('channels/791548/publish/EV5C1YH9KFJ88CKQ', 'field1=nbla&field2=65');
})

client.on('message', function (topic, message) {
  // message is Buffer
  console.log(topic)
  console.log(message.toString())
  client.end()
})