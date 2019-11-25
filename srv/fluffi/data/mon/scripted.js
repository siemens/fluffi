/* global _ */
/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Junes Najah, Thomas Riedmaier
*/

/*
 * Complex scripted dashboard
 * This script generates a dashboard object that Grafana can load. It also takes a number of user
 * supplied URL parameters (in the ARGS variable)
 *
 * Return a dashboard object, or a function
 *
 * For async scripts, return a function, this function must take a single callback function as argument,
 * call this callback function with the dashboard object (look at scripted_async.js for an example)
 */

'use strict';

// accessible variables in this scope
var window, document, ARGS, $, jQuery, moment, kbn;

// Setup some variables
var dashboard;

// All url parameters are available via the ARGS object
var ARGS;

// Initialize a skeleton with nothing but a rows array and service object
dashboard = {
  rows : [],
};


// Set default time
// time can be overridden in the url using from/to parameters, but this is
// handled automatically in grafana core during dashboard initialization
dashboard.time = {
  from: "now-6h",
  to: "now"
};

var fuzzjobName = '';

if(!_.isUndefined(ARGS.fuzzjobname)) {
  fuzzjobName = ARGS.fuzzjobname;
}

// Set a title
dashboard.title = fuzzjobName ? fuzzjobName : "Missing fuzzjobname";

dashboard.rows.push({
    title: 'Fluffi',
    height: '300px',
    panels: [
      // 1st panel 
      {
        title: 'Covered Blocks and Population Size',
        type: 'graph',
        span: 6,
        fill: 1,
        linewidth: 2,
        targets: [
        {
          "groupBy": [
            {
              "params": [
                "$__interval"
              ],
              "type": "time"
            },
            {
              "params": [
                "none"
              ],
              "type": "fill"
            }
          ],
          "measurement": fuzzjobName + "_covered_blocks",
          "orderByTime": "ASC",
          "policy": "default",
          "refId": "A",
          "resultFormat": "time_series",
          "select": [
            [
              {
                "params": [
                  "value"
                ],
                "type": "field"
              },
              {
                "params": [],
                "type": "mean"
              }
            ]
          ],
          "tags": []
        },
        {
          "groupBy": [
            {
              "params": [
                "$__interval"
              ],
              "type": "time"
            },
            {
              "params": [
                "none"
              ],
              "type": "fill"
            }
          ],
          "measurement": fuzzjobName + "_population",
          "orderByTime": "ASC",
          "policy": "default",
          "refId": "B",
          "resultFormat": "time_series",
          "select": [
            [
              {
                "params": [
                  "value"
                ],
                "type": "field"
              },
              {
                "params": [],
                "type": "mean"
              }
            ]
          ],
          "tags": []
        }
      ],
        seriesOverrides: [
          {
            alias: '/random/',
            yaxis: 2,
            fill: 0,
            linewidth: 5
          }
        ],
        tooltip: {
          shared: true
        }
      },
      // 2nd panel
      {
        title: 'Unique Crashes and Violations',
        type: 'graph',
        span: 6,
        fill: 1,
        linewidth: 2,
        targets: [
          {
            "groupBy": [
              {
                "params": [
                  "$__interval"
                ],
                "type": "time"
              },
              {
                "params": [
                  "none"
                ],
                "type": "fill"
              }
            ],
            "measurement": fuzzjobName + "_unique_crashes",
            "orderByTime": "ASC",
            "policy": "default",
            "refId": "A",
            "resultFormat": "time_series",
            "select": [
              [
                {
                  "params": [
                    "value"
                  ],
                  "type": "field"
                },
                {
                  "params": [],
                  "type": "mean"
                }
              ]
            ],
            "tags": []
          },
          {
            "groupBy": [
              {
                "params": [
                  "$__interval"
                ],
                "type": "time"
              },
              {
                "params": [
                  "none"
                ],
                "type": "fill"
              }
            ],
            "measurement": fuzzjobName + "_unique_violations",
            "orderByTime": "ASC",
            "policy": "default",
            "refId": "B",
            "resultFormat": "time_series",
            "select": [
              [
                {
                  "params": [
                    "value"
                  ],
                  "type": "field"
                },
                {
                  "params": [],
                  "type": "mean"
                }
              ]
            ],
            "tags": []
          }
        ],
        seriesOverrides: [
          {
            alias: '/random/',
            yaxis: 2,
            fill: 0,
            linewidth: 5
          }
        ],
        tooltip: {
          shared: true
        }
      },
      // 3rd panel
      {
        title: 'Accumulated Crashes',
        type: 'graph',
        span: 6,
        fill: 1,
        linewidth: 2,
        targets: [
          {
            "groupBy": [
              {
                "params": [
                  "$__interval"
                ],
                "type": "time"
              },
              {
                "params": [
                  "none"
                ],
                "type": "fill"
              }
            ],
            "measurement": fuzzjobName + "_total_violations",
            "orderByTime": "ASC",
            "policy": "default",
            "refId": "A",
            "resultFormat": "time_series",
            "select": [
              [
                {
                  "params": [
                    "value"
                  ],
                  "type": "field"
                },
                {
                  "params": [],
                  "type": "mean"
                }
              ]
            ],
            "tags": []
          },
          {
            "groupBy": [
              {
                "params": [
                  "$__interval"
                ],
                "type": "time"
              },
              {
                "params": [
                  "none"
                ],
                "type": "fill"
              }
            ],
            "measurement": fuzzjobName + "_total_hangs",
            "orderByTime": "ASC",
            "policy": "default",
            "refId": "B",
            "resultFormat": "time_series",
            "select": [
              [
                {
                  "params": [
                    "value"
                  ],
                  "type": "field"
                },
                {
                  "params": [],
                  "type": "mean"
                }
              ]
            ],
            "tags": []
          },
          {
            "groupBy": [
              {
                "params": [
                  "$__interval"
                ],
                "type": "time"
              },
              {
                "params": [
                  "none"
                ],
                "type": "fill"
              }
            ],
            "measurement": fuzzjobName + "_total_crashes",
            "orderByTime": "ASC",
            "policy": "default",
            "refId": "C",
            "resultFormat": "time_series",
            "select": [
              [
                {
                  "params": [
                    "value"
                  ],
                  "type": "field"
                },
                {
                  "params": [],
                  "type": "mean"
                }
              ]
            ],
            "tags": []
          }
        ],
        seriesOverrides: [
          {
            alias: '/random/',
            yaxis: 2,
            fill: 0,
            linewidth: 5
          }
        ],
        tooltip: {
          shared: true
        }
      },
      // 4th Panel
      {
        title: 'System CPU Utilization',
        type: 'graph',
        span: 6,
        fill: 1,
        linewidth: 2,
        targets: [
          {
            "groupBy": [
              {
                "params": [
                  "$__interval"
                ],
                "type": "time"
              },
              {"params": ["server"],"type": "tag"},
              {
                "params": [
                  "none"
                ],
                "type": "fill"
              }
            ],
            "measurement": fuzzjobName + "_sys_cpu_util",
            "orderByTime": "ASC",
            "policy": "default",
            "refId": "A",
            "resultFormat": "time_series",
            "select": [
              [
                {
                  "params": [
                    "value"
                  ],
                  "type": "field"
                },
                {
                  "params": [],
                  "type": "mean"
                }
              ]
            ],
            "tags": [{"key": "server","operator": "=~","value": "/p*/"}]
          }
        ],
        seriesOverrides: [
          {
            alias: '/random/',
            yaxis: 2,
            fill: 0,
            linewidth: 5
          }
        ],
        tooltip: {
          shared: true
        }
      },
      // 5th Panel
      {
        title: 'Executions per Second',
        type: 'graph',
        span: 4,
        fill: 1,
        stack: true,
        linewidth: 2,
        bars: true,
        points: false,
        lines: false,
        targets: [
          {
            "groupBy": [
              {
                "params": [
                  "$__interval"
                ],
                "type": "time"
              },
              {
                "params": [
                  "hostAndPort"
                ],
                "type": "tag"
              },
              {
                "params": [
                  "0"
                ],
                "type": "fill"
              }
            ],
            "measurement": fuzzjobName + "_tc_per_second",
            "orderByTime": "ASC",
            "policy": "default",
            "refId": "B",
            "resultFormat": "time_series",
            "select": [
              [
                {
                  "params": [
                    "value"
                  ],
                  "type": "field"
                },
                {
                  "params": [],
                  "type": "mean"
                }
              ]
            ],
            "tags": [
              {
                "key": "hostAndPort",
                "operator": "=~",
                "value": "/p*/"
              }
            ]
          }
        ],
        seriesOverrides: [
          {
            alias: '/random/',
            yaxis: 2,
            fill: 0,
            linewidth: 5
          }
        ],
        tooltip: {
          shared: true,
          sort: 0,
          value_type: "cumulative"
        }
      },
      // 6th Panel
      {
        title: 'Combined Queue Size Generator',
        type: 'graph',
        span: 4,
        fill: 1,
        linewidth: 2,        
        targets: [
          {
            "groupBy": [
              {
                "params": [
                  "$__interval"
                ],
                "type": "time"
              },
              {
                "params": [
                  "hostAndPort"
                ],
                "type": "tag"
              },
              {
                "params": [
                  "none"
                ],
                "type": "fill"
              }
            ],
            "measurement": fuzzjobName + "_queue_size",
            "orderByTime": "ASC",
            "policy": "default",
            "refId": "B",
            "resultFormat": "time_series",
            "select": [
              [
                {
                  "params": [
                    "value"
                  ],
                  "type": "field"
                },
                {
                  "params": [],
                  "type": "mean"
                }
              ]
            ],
            "tags": [
              {
                "key": "hostAndPort",
                "operator": "=~",
                "value": "/p*/"
              }
            ]
          }
        ],
        seriesOverrides: [
          {
            alias: '/random/',
            yaxis: 2,
            fill: 0,
            linewidth: 5
          }
        ],
        tooltip: {
          shared: true          
        }
      },
      // 7th Panel 
      {
        title: 'Combined Queue Size Evaluator',
        type: 'graph',
        span: 4,
        fill: 1,
        linewidth: 2,        
        targets: [
          {
            "groupBy": [
              {
                "params": [
                  "$__interval"
                ],
                "type": "time"
              },
              {
                "params": [
                  "hostAndPort"
                ],
                "type": "tag"
              },
              {
                "params": [
                  "none"
                ],
                "type": "fill"
              }
            ],
            "measurement": fuzzjobName + "_evaluations_queue_size",
            "orderByTime": "ASC",
            "policy": "default",
            "refId": "A",
            "resultFormat": "time_series",
            "select": [
              [
                {
                  "params": [
                    "value"
                  ],
                  "type": "field"
                },
                {
                  "params": [],
                  "type": "mean"
                }
              ]
            ],
            "tags": [
              {
                "key": "hostAndPort",
                "operator": "=~",
                "value": "/p*/"
              }
            ]
          }
        ],
        seriesOverrides: [
          {
            alias: '/random/',
            yaxis: 2,
            fill: 0,
            linewidth: 5
          }
        ],
        tooltip: {
          shared: true          
        }
      }      
    ]
  });



return dashboard;





