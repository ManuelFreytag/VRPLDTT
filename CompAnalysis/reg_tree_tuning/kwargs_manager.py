"""
Implementation of a keyword arguments manager object.

The keyword argument manager has two main responsibilities:
 1) Translate instance arguments from kwargs format (heuristic) to dummy encoded input (tuner)
 2) Generate random instances from certain value domains

"""
import numbers
import random


class Attribute:
    def __init__(self, name, val_range, dummyname):
        self.name = name
        self.val_range = val_range
        self.__dummyname__ = dummyname

    def __repr__(self):
        return str(self.__dict__)


class NumAttribute(Attribute):
    def __init__(self, name, val_range):
        super().__init__(name, val_range, dummyname=name)

    def get_rnd_value(self):
        upper_val = self.val_range[1]
        lower_val = self.val_range[0]
        return random.random() * (upper_val - lower_val) + lower_val


class CategoricalAttribute(Attribute):
    def __init__(self, name, val_range, category, multi: bool):
        super().__init__(name, val_range, dummyname=category)
        self.category = category
        self.multi = multi

    @staticmethod
    def get_rnd_value():
        return random.random()


class KwargsManager:
    """
    The dummy encoder class allows the handling of categorical data to be tuned.
    It allows categorical attributes (multi-selection, single selection) and numerical data.

    Categorical data is dummy encoded.
    This is a helper functionality allowing to generate
    random samples and train ML models / insert kwargs into the metaheuristic
    """
    attributes = []

    def __repr__(self):
        return str(self.attributes)

    def fit(self, x):
        """
        Fit the Dummy encoder object based on certain values.
        """
        attributes = []
        for key in x:
                if isinstance(x[key], tuple):
                    for category in x[key]:
                        new_att = CategoricalAttribute(name=key,
                                                       val_range=[0, 1],
                                                       category=category,
                                                       multi=False)

                        attributes.append(new_att)

                elif isinstance(x[key], list):
                    if isinstance(x[key][0], str) or isinstance(x[key][0], bool):
                        for category in x[key]:
                            new_att = CategoricalAttribute(name=key,
                                                           val_range=[0, 1],
                                                           category=category,
                                                           multi=True)
                            attributes.append(new_att)

                    elif isinstance(x[key][0], numbers.Number):
                        new_att = NumAttribute(name=key,
                                               val_range=x[key])

                        attributes.append(new_att)
                    else:
                        raise ValueError("No known subtype (numerical, string, boolean)")

                else:
                    raise ValueError("Ranges must be passed as tuples or lists")
        self.attributes = attributes

    def get_rnd_instance(self):
        """
        Get random instance in dummy format
        """
        values = []
        # 1) Get values
        name, max_val, max_id, category = None, None, None, None

        for aid, attr in enumerate(self.attributes):
            if isinstance(attr, NumAttribute):
                # 0) Single selection numerical attributes
                values.append(attr.get_rnd_value())
            elif isinstance(attr, CategoricalAttribute):
                if attr.multi:
                    # 1) Multi-choice attributes (categorical)
                    values.append(round(attr.get_rnd_value()))
                else:
                    # 2) Single choice attributes
                    # Objey exclusivity rules
                    prob = attr.get_rnd_value()

                    # 2.1) First entry of a new attribute
                    if name != attr.name:
                        name, max_val, max_id = attr.name, prob, aid
                        values.append(1)
                    # 2.2) Better selection of a new attribute
                    elif max_val < prob:
                        values[max_id] = 0
                        name, max_val, max_id = attr.name, prob, aid
                        values.append(1)
                    else:
                        values.append(0)

        return values

    def dummy_to_kwargs(self, dummy_instance):
        """
        Translate data from dummy format to kwargs format
        """
        if len(dummy_instance) != len(self.attributes):
            raise ValueError("Instance does not fit the expected nr attributes")

        kwargs = {}
        for aid, attr in enumerate(self.attributes):
            if isinstance(attr, NumAttribute):
                kwargs[attr.name] = dummy_instance[aid]
            elif isinstance(attr, CategoricalAttribute):
                # 1) Multichoice attributes are returned in a list (at least empty list)
                # 2) Singlechoice attributes are returned as native dtype
                try:
                    kwargs[attr.name]
                except KeyError:
                    kwargs[attr.name] = []

                if dummy_instance[aid] == 1:
                    if attr.multi:
                        kwargs[attr.name].append(attr.category)  # add to list
                    else:
                        kwargs[attr.name] = attr.category  # native
            else:
                raise ValueError("One or more attributes are neither categorical nor numerical")
        return kwargs

    def get_domains(self):
        """
        Utility function to retrieve the allowed domains by the keywords manager
        Relevant to get the new setting after tuning all domains

        Returns: Dictionary of domains

        """
        domains = {}

        for attribute in self.attributes:
            if isinstance(attribute, NumAttribute):
                # Domain is strictly known
                domains[attribute.__dummyname__] = attribute.val_range

            elif isinstance(attribute, CategoricalAttribute):
                # Add !allowed! categories to the domain list
                try:
                    curr_domain = domains[attribute.name]
                except KeyError:
                    curr_domain = {"sig_good": [], "sig_bad": [], "not_sig": []}

                if attribute.val_range[0] >= 0.5:
                    curr_domain["sig_good"].append(attribute.__dummyname__)
                if attribute.val_range[1] <= 0.5:
                    curr_domain["sig_bad"].append(attribute.__dummyname__)
                else:
                    curr_domain["not_sig"].append(attribute.__dummyname__)

                domains[attribute.name] = curr_domain

            else:
                raise ValueError(f"Unexpected error. Attribute {attribute.name} is not of known type")

        return domains
