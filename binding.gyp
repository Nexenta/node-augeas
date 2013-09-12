{
  'targets': [{
      'target_name': 'augeas',
      'sources': ['libaugeas.cc'],
      'conditions': [
        ['OS=="mac"', {
            'include_dirs': [
                '/usr/local/Cellar/libxml2/2.9.1/include/libxml2',
                '/usr/local/Cellar/augeas/1.0.0/include'
            ],
            'libraries': [
                '-laugeas'
            ]
          }
        ],
        ['OS=="linux"', {
            'include_dirs': [
                '/usr/include/libxml2'
            ],
            'libraries': [
                '-laugeas'
            ]
          }
        ]
      ]
    }
  ]
}
