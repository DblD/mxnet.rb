require 'mxnet/gluon/block'
require 'mxnet/gluon/nn'

module MXNet::Gluon::NN
  ##
  # Just your regular densely-connected neural network layer.
  #
  # Implements the operation:
  #
  #     output = activation(dot(input, weight) + bias)
  #
  # where `activation` is the element-wise activation function passed
  # as the "activation" argument, `weight` is a weights matrix created
  # by the layer, and `bias` is a bias vector created by the layer
  # (if "use_bias" is +true+).
  #
  # Note: "input" must be a tensor with rank two. Use "flatten" to
  # convert it to rank two if necessary.
  class Dense < MXNet::Gluon::HybridBlock
    ##
    # Creates a new instance.
    #
    # ====Parameters
    #
    # +units_::      (integer)
    #                Dimensionality of the output space.
    # +use_bias+::   (boolean, default +true+)
    #                Whether the layer uses a bias vector.
    # +activation+:: (string, optional)
    #                Activation function to use. If nothing is
    #                specified, no activation is applied (it acts like
    #                "linear" activation: `a(x) = x`).
    # +in_units+::   (integer, optional)
    #                Size of the input data. If not specified,
    #                initialization will be deferred to the first time
    #                #forward is called and `in_units` will be
    #                inferred from the shape of input data.
    #
    def initialize(units, use_bias: true, activation: nil, in_units: 0, **kwargs)
      super(**kwargs)
      with_name_scope do
        @units = units
        @use_bias = use_bias
        @in_units = in_units
        self[:weight] = params.get(
          'weight',
          shape: [units, in_units],
          allow_deferred_init: true
        )
        if @use_bias
          self[:bias] = params.get(
            'bias',
            shape: [units],
            allow_deferred_init: true
          )
        else
          self[:bias] = nil
        end
      end
      if activation
        self[:act] = MXNet::Gluon::NN::Activation(
          activation,
          prefix: "#{activation}_"
        )
      else
        self[:act] = nil
      end
    end

    def hybrid_forward(clazz, data, weight=nil, bias = nil, **kwargs)
      if (weight ||= kwargs[:weight]).nil?
        raise ArgumentError, "weight is not given"
      end
      bias ||= kwargs[:bias]
      out = clazz.FullyConnected(
        data,
        weight,
        bias,
        no_bias: bias.nil?,
        num_hidden: @units
      )
      if self[:act]
        out = self[:act][out]
      end
      out
    end
  end
  def self.Dense(*args)
    Dense.new(*args)
  end
end
