import React, {useState, useEffect, useLayoutEffect, useRef} from 'react';
import './App.css';

import io from 'socket.io-client';

var searchParams = new URLSearchParams(window.location.search);

if (!searchParams.has('port')) {
  window.alert(`'port' url query param required`);
  throw new Error(`'port' url query param required`);
}

const socketPort = parseInt(searchParams.get('port'));

// a synchronously inspectable promise wrapper
class Future {
  state = 'pending';
  value = null;
  error = null;

  constructor(promise) {
    this.promise = promise;
    promise
      .then((value) => {
        this.state = 'fulfilled';
        this.value = value;
      })
      .catch((err) => {
        this.state = 'rejected';
        this.error = err;
      });
  }
}

class RequestCache {
  items = new Map();

  get(url, makeRequest) {
    if (!this.items.has(url)) {
      this.items.set(url, new Future(makeRequest(url)));
    }

    return this.items.get(url);
  }
}

const requestCache = new RequestCache();

function Log({logItems}) {
  const logEl = useRef(null);

  useLayoutEffect(() => {
    logEl.current.scrollTop = logEl.current.scrollHeight;
  }, [logItems]);

  return (
    <div style={{overflowY: 'scroll', height: 400}} ref={logEl}>
      {logItems.map((logItem, i) => (
        <div key={i}>{logItem}</div>
      ))}
    </div>
  );
}

function Disassembly({currentAddress, disassembly}) {
  const dataForAddress = disassembly.addresses[currentAddress];
  const addrsForFn =
    dataForAddress && dataForAddress.fnName
      ? disassembly.functions[dataForAddress.fnName]
      : null;

  if (!addrsForFn) {
    if (dataForAddress) {
      return (
        <div>
          {currentAddress}: {dataForAddress.disasm} (fnName not found)
        </div>
      );
    } else {
      return <div>{currentAddress}: (dataForAddress not found)</div>;
    }
  }
  const addrsForFnSorted = addrsForFn
    .slice(0)
    .sort((a, b) => parseInt(a, 16) - parseInt(b, 16));
  const posInAddrs = addrsForFnSorted.indexOf(currentAddress);
  const addrForContext = addrsForFnSorted.slice(
    Math.max(0, posInAddrs - 5),
    Math.min(addrsForFnSorted.length, posInAddrs + 6)
  );
  return (
    <div style={{border: 'solid 1px black'}}>
      {addrForContext.map((addr) => {
        const dataForAddr = disassembly.addresses[addr];

        let result = null;
        if (!dataForAddr) {
          result = <span>{addr}: unknown</span>;
        } else {
          // table row
          // TODO: render as columns
          result = (
            <div key={addr} style={{overflow: 'hidden', whiteSpace: 'nowrap'}}>
              <span style={{display: 'inline-block', width: 60}}>
                {dataForAddr.offset}
              </span>{' '}
              <span title={dataForAddr.disasm}>{dataForAddr.disasm}</span>
            </div>
          );
        }

        return (
          <div
            key={addr}
            style={{color: addr === currentAddress ? 'red' : null}}
          >
            {result}
          </div>
        );
      })}
    </div>
  );
}

function Stacktrace({pc, stacktrace}) {
  const [disassembly, setDisassembly] = useState(null);
  useEffect(() => {
    requestCache
      .get(
        `${window.location.protocol}//${window.location.hostname}:${socketPort}/disassembly`,
        async (url) => {
          const body = await fetch(url);
          const data = await body.json();
          console.log(data);
          return data;
        }
      )
      .promise.then((result) => setDisassembly(result));
  }, []);

  if (disassembly) {
    const pc = stacktrace[0];
    const pcData = disassembly.addresses[pc];
    return (
      <div style={{overflow: 'auto'}}>
        {pc && (
          <div key={pc}>
            <div>{pcData && `at: ${pcData.fnName}${pcData.offset}`}</div>
            <Disassembly currentAddress={pc} disassembly={disassembly} />
          </div>
        )}
        {stacktrace.slice(1).map((stackframe) => {
          const frameData = disassembly.addresses[stackframe];
          return (
            <div key={stackframe}>
              <details>
                <summary>
                  {frameData
                    ? `in: ${frameData.fnName}`
                    : `in: [unknown function at ${stackframe}]`}
                </summary>

                <Disassembly
                  currentAddress={stackframe}
                  disassembly={disassembly}
                />
              </details>
            </div>
          );
        })}
      </div>
    );
  }

  return 'loading...';
}

const Threads = React.memo(function Threads({threads, threadIDAtBreak, api}) {
  return (
    <div className="threads-grid">
      {Object.values(threads)
        .sort((a, b) => a.id - b.id)
        .map((thread) => (
          <div key={thread.id}>
            <h3 style={{height: 24, overflow: 'hidden', margin: '8px 0 0 0'}}>
              thread {thread.id}
              {thread.id === threadIDAtBreak && (
                <span>
                  {' '}
                  <button onClick={() => api.sendCommand('s')}>step</button>
                  <button onClick={() => api.sendCommand('r')}>run</button>
                </span>
              )}
            </h3>
            <div style={{fontSize: 12, height: '1.2em', fontStyle: 'italic'}}>
              {' '}
              {thread.id === threadIDAtBreak && '(at breakpoint)'}{' '}
            </div>
            <div>state: {thread.stateName}</div>
            <div>priority: {thread.priority}</div>
            <div>pc: {thread.pc}</div>
            <div>
              <Stacktrace pc={thread.pc} stacktrace={thread.stacktrace} />
            </div>
          </div>
        ))}
    </div>
  );
});

function App() {
  const [state, setState] = useState(null);
  const [logItems, setLogItems] = useState([]);

  let apiRef = useRef(null);

  useEffect(() => {
    const socket = io.connect(`http://localhost:${socketPort}`);
    socket.on('state', (newState) => {
      console.log(newState);
      setState(newState);
    });
    socket.on('log', (newLogItem) => {
      setLogItems((logItems) => logItems.concat(newLogItem));
    });

    apiRef.current = {
      sendCommand(cmd, data) {
        socket.emit('cmd', {cmd, data});
      },
    };
  }, []);

  if (!state) {
    return <div style={{margin: 100}}>awaiting initial state...</div>;
  }

  return (
    <div>
      {state.serverErrors && (
        <div style={{backgroundColor: 'red', color: 'white'}}>
          {state.serverErrors.map(({message, error}, i) => (
            <div>{message}</div>
          ))}
        </div>
      )}
      <div style={{display: 'flex'}}>
        <div className="pane-thread">
          <h2>threads</h2>
          <Threads
            threads={state.threads}
            threadIDAtBreak={state.atBreakpoint}
            api={apiRef.current}
          />
        </div>
        <div className="pane-log">
          <h2>log</h2>
          <Log logItems={logItems} />
        </div>
      </div>
      <details>
        <summary>state</summary>
        <pre>{JSON.stringify(state, null, 2)}</pre>
      </details>
    </div>
  );
}

export default App;
