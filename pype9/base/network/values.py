from pyNN.parameters import Sequence
from pyNN.random import RandomDistribution
from nineml.values import SingleValue, ArrayValue, RandomValue

random_value_map = {
    'http://www.uncertml.org/distributions/uniform':
    ('uniform', ('minimum', 'maximum')),
    'http://www.uncertml.org/distributions/normal':
    ('normal', ('mean', 'variance')),
    'http://www.uncertml.org/distributions/exponential':
    ('exponential', ('rate',)),
    'http://www.uncertml.org/distributions/binomial':
    ('binomial', ('n', 'p')),
    'http://www.uncertml.org/distributions/gamma':
    ('gamma', ('k', 'theta')),
    'http://www.uncertml.org/distributions/lognormal':
    ('lognormal', ('mu', 'sigma'))}


def get_pyNN_value(qty, unit_handler, rng):
    if isinstance(qty.value, SingleValue):
        val = unit_handler.scale_value(qty.quantity)
    elif isinstance(qty.value, ArrayValue):
        scalar = unit_handler.scalar(qty.units)
        val = Sequence(v * scalar for v in qty.value)
    elif isinstance(qty.value, RandomValue):
        if unit_handler.scalar(qty.units) != 1.0:
            raise NotImplementedError(
                "Cannot currently scale random distributions as required to "
                "{} get into the correct units".format(qty))
        try:
            rv_name, rv_param_names = random_value_map[
                qty.value.distribution.standard_library]
        except KeyError:
            raise NotImplementedError(
                "Sorry, '{}' random distributions are not currently supported"
                .format(qty.value.distribution.standard_libary))
        rv_params = [
            qty.value.distribution.property(n).value for n in rv_param_names]
        # FIXME: Need to scale random distribution to correct units. Should
        #        probably derive PyNN RandomDistribution class to multiply by
        #        when a value is drawn
        val = RandomDistribution(rv_name, rv_params, rng=rng)
    return val
