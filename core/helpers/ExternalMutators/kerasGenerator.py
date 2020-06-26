# Copyright 2017-2020 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including without
# limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
# SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# 
# Author(s): Abian Blome

import numpy as np
import pandas as pd
from keras.utils import to_categorical
from keras.preprocessing.sequence import pad_sequences
from keras.models import Sequential
from keras.layers import LSTM, Dense, GRU, Embedding
from keras.callbacks import EarlyStopping, ModelCheckpoint
from sklearn.model_selection import train_test_split

from sqlalchemy import create_engine, text
import sys, os, time, uuid, random, datetime


# Simple generator that uses the information available in the database to generate samples
# We currently don't have a good way of passing options, so we do it over the following set of variables

fuzzjobName = "MyParser"
extDir = "c:\\temp\\generators"
database_string = 'mysql+pymysql://user:pass@dbserver:3306/fluffi_gm'
limit = "ORDER BY RAND() LIMIT 2000"
epochs = 20

# Now we are ready to start
guid = str(uuid.uuid4())
localID = 0

def registerInDB(con):
    global guid
    global extDir
    diffMutatorsResp = con.execute('SELECT * FROM nice_names_managed_instance WHERE NiceName LIKE "KerasGenerator %%"')
    if diffMutatorsResp.rowcount == 0:
        niceName = "KerasGenerator 1"
    else:
        maxVal = 0
        for diffMutator in diffMutatorsResp:
            val = int(diffMutator[1].split(" ")[1])
            if val > maxVal:
                maxVal = val
        niceName = "KerasGenerator %d" % (maxVal+1)
    insertQuery = "INSERT INTO nice_names_managed_instance(NiceName, ServiceDescriptorGUID) VALUES(:nicename, :guid)"
    con.execute(text(insertQuery), {
        'guid': guid,
        'nicename': niceName
    } )
    os.mkdir(os.path.join(extDir, guid))

def get_engine(database_string):
    engine = create_engine(database_string)
    with engine.connect() as con:
        fuzzjobsDB = con.execute('SELECT * FROM fuzzjob WHERE name="%s"' % fuzzjobName)
    if fuzzjobsDB.rowcount != 1:
        print("Could not find %s" % fuzzjobName)
        sys.exit(1)
    selected_fuzzjob = next(fuzzjobsDB)

    hostname = selected_fuzzjob[2]
    username = selected_fuzzjob[3]
    password = selected_fuzzjob[4]
    dbname = selected_fuzzjob[5]

    database_string = 'mysql+pymysql://%s:%s@%s/%s' % (username, password, hostname, dbname)
    engine = create_engine(database_string)
    return engine

def get_maxdate(engine):
    with engine.connect() as con:
          maxdate = con.execute('SELECT MAX(timeofinsertion) FROM interesting_testcases')
    if maxdate.rowcount != 1:
        print("Could not find get maxdate")
        sys.exit(1)
    return next(maxdate)[0]

def get_testcases(engine, limit, date=None):
    with engine.connect() as con:
        if date is None:
            testcaseResp = con.execute('SELECT * FROM interesting_testcases WHERE TestCaseType=0 %s' % limit)
        else:
            testcaseResp = con.execute('SELECT * FROM interesting_testcases WHERE TestCaseType=0 AND TimeOfInsertion > %s %s' % (date, limit))   
    testcases = []
    for testcase in testcaseResp:
        testcases.append(testcase[6])
    return testcases

def createSeq(data, sequences, length=40):
    for i in range(length, len(data)):
        seq = data[i-length:i+1]
        sequences.append(seq)
    print('Total Sequences: %d' % len(sequences))
    return sequences

def encode_seq(mapping, seq):
    sequences = list()
    for line in seq:
        # integer encode line
        encoded_seq = [mapping[char] for char in line]
        # store
        sequences.append(encoded_seq)
    return np.array(sequences)

def getCharset(testcases):
    # create a character mapping index
    chars = list()
    charset = set()
    for testcase in testcases:
        charset = charset.union(set(testcase))
    chars = sorted(list(charset))
    return chars

def getMapping(chars):
    mapping = dict((c, i) for i, c in enumerate(chars))
    return mapping

def getModel(numChars):
    model = Sequential()
    model.add(Embedding(numChars, 256, input_length=40, trainable=True))
    model.add(GRU(150, recurrent_dropout=0.1, dropout=0.1))
    model.add(Dense(numChars, activation='softmax'))
    print(model.summary())
    model.compile(loss='categorical_crossentropy', metrics=['acc'], optimizer='adam')
    return model

def getFeatures(testcases):
    sequences = list()
    for testcase in testcases:
        sequences = createSeq(testcase, sequences)
    return sequences
  
def sample_prediction(char_map, prediction):
	rnd_idx = np.random.choice(len(prediction), p=prediction)
	return char_map[rnd_idx]

# generate a sequence of characters with a language model
def generate_seq(model, mapping, seq_length, seed_text, n_chars):
	in_text = seed_text
	# generate a fixed number of characters
	for _ in range(n_chars):
		# encode the characters as integers
		encoded = [mapping[char] for char in in_text]
		# truncate sequences to a fixed length
		encoded = pad_sequences([encoded], maxlen=seq_length, truncating='pre')
		# predict character
		prediction = model.predict(encoded, verbose=0)
		yhat=np.random.choice(len(prediction[0]), p=prediction[0])
		#yhat = model.predict_classes(encoded, verbose=0)
		# reverse map integer to character
		out_char = ''
		for char, index in mapping.items():
			if index == yhat:
				out_char = char
				break
		# append to input
		in_text += (char,)
	return in_text

def getMaxSize(testcases):
    maxSize = 0
    for testcase in testcases:
        if maxSize < len(testcase):
            maxSize = len(testcase)
    return maxSize

engine = get_engine(database_string)
registerInDB(engine)
maxdate = get_maxdate(engine)
testcases = get_testcases(engine, limit)
sequences = getFeatures(testcases)
charset = getCharset(testcases)
mapping = getMapping(charset)
sequences = encode_seq(mapping, sequences)
maxSize = getMaxSize(testcases)
curIndex = 0
model = getModel(len(mapping))

x, y = sequences[:,:-1], sequences[:,-1]
y = to_categorical(y, num_classes=len(mapping))
x_tr, x_val, y_tr, y_val = train_test_split(x, y, test_size=0.1, random_state=42)

model.fit(x_tr, y_tr, epochs=2, validation_data=(x_val, y_val))
lastTrain = datetime.datetime.now()

while True:
    # Check if enough time has passed, if so then retrain our model with (hopefully) new testcases (every 4 hours?)
    if (datetime.datetime.now() - lastTrain).total_seconds() > 60*60*4:
        # For now we do it quick & ugly:
        maxdate = get_maxdate(engine)
        testcases = get_testcases(engine)
        sequences = getFeatures(testcases)
        mapping = getMapping(testcases)
        sequences = encode_seq(mapping, sequences)
        maxSize = getMaxSize(testcases)
        model = getModel(len(mapping))

        x, y = sequences[:,:-1], sequences[:,-1]
        y = to_categorical(y, num_classes=len(mapping))
        x_tr, x_val, y_tr, y_val = train_test_split(x, y, test_size=0.1, random_state=42)

        model.fit(x_tr, y_tr, epochs=epochs, verbose=2, validation_data=(x_val, y_val))
        lastTrain = datetime.datetime.now()

    numFiles = len([name for name in os.listdir(extDir)])
    if numFiles >= 50:
        continue
    print("Generating new sequence")
    new_seq= generate_seq(model, mapping, 40, (random.choice(charset),), random.randint(0, maxSize)) 
    filename = "%s_%d_%d" % ("sampleGenerator", 0, curIndex)
    with open(os.path.join(extDir, filename), "wb") as f:
        f.write(bytearray(new_seq))
    curIndex+=1
