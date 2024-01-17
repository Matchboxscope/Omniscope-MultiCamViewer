#deploy this to some lcoud hosted server (for example on heroku), the esp32 will not see it in the localhost
#put the following template in the templates folder besides this file
from flask import Flask, request, render_template
import logging
import os

app = Flask(__name__)

'''
logging.basicConfig(level=logging.DEBUG) #for better debuging, we will log out every request with headers and body.
@app.before_request
def log_request_info():
    logging.info('Headers: %s', request.headers)
    logging.info('Body: %s', request.get_data())
'''

@app.route("/port")
def port():
    print(request.remote_addr)
    print(request.get_data())
    return "5001"

@app.route("/upload", methods=["GET", "POST"])
def upload():

    if request.method == "POST": #if we make a post request to the endpoint, look for the image in the request body
        image_raw_bytes = request.get_data()  #get the whole body

        save_location = (os.path.join(app.root_path, "static/"+request.remote_addr+".jpg")) #save to the same folder as the flask app live in 

        f = open(save_location, 'wb') # wb for write byte data in the file instead of string
        f.write(image_raw_bytes) #write the bytes from the request body to the file
        f.close()

        print("Image saved")

        return "image saved"

    if request.method == "GET": #if we make a get request (for example with a browser) show the image.
# The browser will cache this image so when you want to actually refresh it, press ctrl-f5
        return render_template("image_show.html")
    return "if you see this, that is bad request method"

app.run(host='0.0.0.0', port=5001)