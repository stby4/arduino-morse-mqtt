const mqtt = require('mqtt')
const request = require('request')

const client = mqtt.connect('mqtt://test.mosquitto.org:1883')
const ifttt_event = 'message_received'
const ifttt_key = 'ieUjJOEVdcUtlLeoXiOfSoOb6N48N73heOroj6lqPLj'

const message_queue = new Object()


console.log('Connecting')

client.on('connect', function () {
  client.subscribe('morse/791548')
  console.log("Connected")
})

client.on('message', function (topic, message) {
  const msg = message.toString()
  console.log(`${topic}: ${msg}`)

  // CSV -> array
  const parts = msg.split(',')
  const sender = parts[0]
  const letter = parts[1]

  // add to queue
  if (message_queue.hasOwnProperty(sender)) {
    message_queue[sender] = `${message_queue[sender]}${letter}`
  } else {
    message_queue[sender] = letter
  }

  // send if STOP string is found
  if (-1 < message_queue[sender].indexOf("STOP")) {
    const json = {
      value1: sender, value2: message_queue[sender].replace('STOP', '')
    }

    const options = {
      uri: `https://maker.ifttt.com/trigger/${ifttt_event}/with/key/${ifttt_key}`,
      method: 'POST',
      json
    }

    console.log(json)
    request(options, (error, response, body) => {
      if (!error && response.statusCode == 200) {
        // remove message queue for this sender
        delete message_queue[sender]
        console.log(body)
      }
      if (error) {
        console.warn(error)
      }
    })
  }
})